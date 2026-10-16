// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "AppForge/Application/ForgeApplication.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QSignalBlocker>
#include <algorithm>
#include <ranges>

#include "AppForge/Component/ComponentFactory.h"
#include "Assembler.h"
#include "Composite/ComponentPaths.h"
#include "ProjectJson.h"

namespace AppForge
{

namespace
{

Q_LOGGING_CATEGORY(lcPlugins, "appforge.plugins")

// The version of the projects written, and of the composite files: 2 has composites. Those of a version from 1 to
// this one are read.
constexpr int ProjectVersion = 2;

// The names of the options addCommandLineOptions() adds.
const QString PluginDirOption = QStringLiteral("plugin-dir");
const QString PluginRootOption = QStringLiteral("plugin-root");

// Whether component is of the plugin pluginId, or is a composite that holds one, at any depth.
bool holds(const Component& component, const QString& pluginId)
{
    if(const auto* composite = qobject_cast<const Composite*>(&component))
    {
        return std::ranges::any_of(composite->components(), [&pluginId](const std::unique_ptr<Component>& inner)
                                   { return holds(*inner, pluginId); });
    }
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    return info && info->pluginId == pluginId;
}

using ComponentPaths::refusalIn;

// entry as a project holds it: the path of its component, see ForgeApplication::path(), the key of its object when it
// is not the component, see ProjectJson::ObjectKey, its "index" only when it is not the first object of its name, and
// its property.
QJsonObject entryJson(const ForgeApplication& application, const SettingsEntry& entry)
{
    QJsonObject json;
    json.insert(u"component", application.path(*entry.component));
    if(const std::optional<ProjectJson::ObjectKey> key = ProjectJson::objectKey(*entry.component, entry.object))
    {
        json.insert(u"object", key->name);
        if(key->index > 0)
        {
            json.insert(u"index", key->index);
        }
    }
    json.insert(u"property", entry.property);
    return json;
}

} // namespace

ForgeApplication::ForgeApplication(QObject* parent)
    : QObject(parent), m_pluginRoot(QCoreApplication::applicationDirPath())
{
    m_plugins.scan();
}

ForgeApplication::~ForgeApplication()
{
    const QSignalBlocker blocker(this); // Not told to anyone: it is going
    clear();
}

PluginManager& ForgeApplication::plugins()
{
    return m_plugins;
}

const PluginManager& ForgeApplication::plugins() const
{
    return m_plugins;
}

QString ForgeApplication::pluginRoot() const
{
    return m_pluginRoot.absolutePath();
}

void ForgeApplication::setPluginRoot(const QString& directory)
{
    m_pluginRoot.setPath(directory);
}

void ForgeApplication::addCommandLineOptions(QCommandLineParser& parser)
{
    parser.addOption(
        {PluginDirOption,
         QStringLiteral("Directory of plugins to scan, after plugins/ next to the application. Can be given "
                        "several times: the directories are scanned in their order, and a plugin found "
                        "again is skipped."),
         QStringLiteral("directory")});
    parser.addOption({PluginRootOption,
                      QStringLiteral("Directory the files of the plugins are relative to in a project file (default: "
                                     "the directory of the application)."),
                      QStringLiteral("directory")});
}

void ForgeApplication::applyCommandLine(const QCommandLineParser& parser)
{
    for(const QString& directory : parser.values(PluginDirOption))
    {
        if(!QFileInfo(directory).isDir())
        {
            qCWarning(lcPlugins).noquote()
                << "Skipping" << QDir::toNativeSeparators(directory) << "- it is not a directory";
            continue;
        }
        m_plugins.scan(directory);
    }
    if(parser.isSet(PluginRootOption))
    {
        setPluginRoot(parser.value(PluginRootOption));
    }
}

std::optional<QJsonObject> ForgeApplication::read(const QString& filePath, QString& error)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        error = file.errorString();
        return std::nullopt;
    }
    QJsonParseError parsed;
    QJsonObject project = QJsonDocument::fromJson(file.readAll(), &parsed).object();
    if(parsed.error != QJsonParseError::NoError)
    {
        error = parsed.errorString();
        return std::nullopt;
    }
    const int version = project.value(u"version").toInt();
    if(version < 1 || version > ProjectVersion)
    {
        error = QStringLiteral("It is not an AppForge project of a version from 1 to %1").arg(ProjectVersion);
        return std::nullopt;
    }
    return project;
}

QStringList ForgeApplication::load(const QJsonObject& project, const QString& directory)
{
    clear();
    Assembler assembler(*this);
    assembler.loadPlugins(project);
    assembler.load(m_components, project, QDir(directory), {});
    QStringList problems = assembler.problems();

    const QJsonArray settings = project.value(u"settings").toArray();
    for(const QJsonValue& value : settings)
    {
        const QJsonObject json = value.toObject();
        SettingsFile& file = addSettingsFile();
        const QString fileName = json.value(u"file").toString();
        // Not the name of one read before: they would be written over each other.
        if(!fileName.isEmpty() && !file.setFileName(fileName))
        {
            problems.append(QStringLiteral("The settings file %1 is named %2: another one has its name")
                                .arg(fileName, file.fileName()));
        }
        file.setDirectory(json.value(u"directory").toString());
        const QJsonArray entries = json.value(u"entries").toArray();
        for(const QJsonValue& entryValue : entries)
        {
            const QJsonObject entry = entryValue.toObject();
            const QString property = entry.value(u"property").toString();
            QString owner = entry.value(u"component").toString(); // What the property is of, as it is told
            Component* instance = component(owner);
            QObject* object = instance;
            if(entry.contains(u"object"))
            {
                const ProjectJson::ObjectKey key{.name = entry.value(u"object").toString(),
                                                 .index = entry.value(u"index").toInteger()};
                object = instance != nullptr ? ProjectJson::object(*instance, key) : nullptr;
                owner = QStringLiteral("the object %1 of %2").arg(key.name, owner);
            }
            if(object == nullptr || !file.add({.component = instance, .object = object, .property = property}))
            {
                problems.append(
                    QStringLiteral("The property %1 of %2 is not held by %3").arg(property, owner, file.fileName()));
            }
        }
    }
    return problems;
}

bool ForgeApplication::open(const QString& filePath)
{
    const std::optional<QJsonObject> project = read(filePath, m_errorString);
    if(!project)
    {
        return false;
    }
    const QStringList problems = load(*project, QFileInfo(filePath).absolutePath());
    m_errorString = problems.join(u'\n');
    return problems.isEmpty();
}

QJsonObject ForgeApplication::save(const QString& directory) const
{
    QJsonObject project = Assembler::save(m_components, directory, m_plugins, m_pluginRoot);
    project.insert(u"version", ProjectVersion);

    QJsonArray settings;
    for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles)
    {
        QJsonObject json;
        json.insert(u"file", file->fileName());
        json.insert(u"directory", file->directory());
        QJsonArray entries;
        for(const SettingsEntry& entry : file->entries())
        {
            entries.append(entryJson(*this, entry));
        }
        json.insert(u"entries", entries);
        settings.append(json);
    }
    if(!settings.isEmpty()) // Only for those who use them: the others are written as they were
    {
        project.insert(u"settings", settings);
    }
    return project;
}

Component* ForgeApplication::instantiate(const QString& componentId, const QString& name)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
    // Not one Core provides: a composite is made from its file.
    std::unique_ptr<Component> instance = info && info->pluginId != ComponentFactory::corePluginId()
                                              ? ComponentFactory::instance().create(componentId)
                                              : nullptr;
    if(instance == nullptr)
    {
        return nullptr;
    }
    instance->setObjectName(
        ComponentPaths::canName(m_components, name) ? name : ComponentPaths::newName(m_components, info->name));
    return m_components.emplace_back(std::move(instance)).get();
}

Composite* ForgeApplication::instantiateComposite(const QString& filePath, const QString& name)
{
    const QString file = Assembler::compositeFilePath(QDir(), filePath);
    Assembler assembler(*this);
    std::unique_ptr<Composite> instance = assembler.instantiate(file, m_errorString);
    if(instance == nullptr)
    {
        return nullptr;
    }
    m_errorString = assembler.problems().join(u'\n');
    const QString base = QFileInfo(file).completeBaseName();
    instance->setObjectName(ComponentPaths::canName(m_components, name) ? name
                                                                        : ComponentPaths::newName(m_components, base));
    Composite& placed = *instance;
    m_components.push_back(std::move(instance));
    return &placed;
}

bool ForgeApplication::rename(Component& instance, const QString& name)
{
    const bool owned = std::ranges::any_of(m_components, [&instance](const std::unique_ptr<Component>& component)
                                           { return component.get() == &instance; });
    if(!owned || !ComponentPaths::canName(m_components, name, &instance))
    {
        return false;
    }
    instance.setObjectName(name);
    return true;
}

void ForgeApplication::remove(Component& instance)
{
    const auto found = std::ranges::find_if(m_components, [&instance](const std::unique_ptr<Component>& owned)
                                            { return owned.get() == &instance; });
    if(found == m_components.end())
    {
        return;
    }
    instance.stop(); // Does nothing to one that does not run
    m_components.erase(found);
}

const std::vector<std::unique_ptr<Component>>& ForgeApplication::components() const
{
    return m_components;
}

Component* ForgeApplication::component(QStringView path) const
{
    return ComponentPaths::find(m_components, path);
}

QString ForgeApplication::path(const Component& component) const
{
    return ComponentPaths::path(m_components, component);
}

QList<Component*> ForgeApplication::componentsOf(const QString& pluginId) const
{
    QList<Component*> found;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(holds(*instance, pluginId))
        {
            found.append(instance.get());
        }
    }
    return found;
}

void ForgeApplication::clear()
{
    stop();
    // The settings files first: their entries go as the components do, which they would be told one by one.
    if(!m_settingsFiles.empty())
    {
        m_settingsFiles.clear();
        emit settingsFilesChanged();
    }
    m_components.clear();
    m_definitions.clear();
}

SettingsFile& ForgeApplication::addSettingsFile()
{
    // Not made with std::make_unique: its constructor is only for the application.
    SettingsFile& file = *m_settingsFiles.emplace_back(new SettingsFile(*this, newSettingsFileName()));
    connect(&file, &SettingsFile::changed, this, &ForgeApplication::settingsFilesChanged);
    connect(&file, &SettingsFile::failed, this, &ForgeApplication::settingsFileFailed);
    emit settingsFilesChanged();
    return file;
}

void ForgeApplication::removeSettingsFile(SettingsFile& file)
{
    const auto found = std::ranges::find_if(m_settingsFiles, [&file](const std::unique_ptr<SettingsFile>& owned)
                                            { return owned.get() == &file; });
    if(found == m_settingsFiles.end())
    {
        return;
    }
    if(m_running)
    {
        file.stop();
    }
    m_settingsFiles.erase(found);
    emit settingsFilesChanged();
}

const std::vector<std::unique_ptr<SettingsFile>>& ForgeApplication::settingsFiles() const
{
    return m_settingsFiles;
}

SettingsFile* ForgeApplication::settingsFile(const SettingsEntry& entry) const
{
    const auto found = std::ranges::find_if(m_settingsFiles, [&entry](const std::unique_ptr<SettingsFile>& file)
                                            { return file->contains(entry); });
    return found != m_settingsFiles.end() ? found->get() : nullptr;
}

SettingsFile* ForgeApplication::settingsFile(const QString& fileName) const
{
    // Whatever its case: two names that only differ by it are the same file where file names do not tell it apart.
    const auto found = std::ranges::find_if(m_settingsFiles, [&fileName](const std::unique_ptr<SettingsFile>& file)
                                            { return file->fileName().compare(fileName, Qt::CaseInsensitive) == 0; });
    return found != m_settingsFiles.end() ? found->get() : nullptr;
}

bool ForgeApplication::start()
{
    // Once, from the moment it starts: when it runs already, the properties have the values of the user, not those of
    // the project.
    const bool starting = !m_running;
    if(starting)
    {
        for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles)
        {
            file->start();
        }
    }
    // The properties have their default values again when it does not run.
    const auto stopSettings = [this, starting]
    {
        if(!starting)
        {
            return;
        }
        for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles | std::views::reverse)
        {
            file->stop();
        }
    };

    // All of them before any is started: one that starts can count on the others being ready.
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(instance->state() == ComponentState::Initializing && !instance->initialize())
        {
            m_errorString = QStringLiteral("%1 could not be initialized").arg(refusalIn(*instance));
            stopSettings();
            return false;
        }
    }
    std::vector<Component*> started;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(instance->state() != ComponentState::Ready)
        {
            continue; // It runs already
        }
        if(!instance->start())
        {
            m_errorString = QStringLiteral("%1 could not be started").arg(refusalIn(*instance));
            for(Component* running : started | std::views::reverse)
            {
                running->stop();
            }
            stopSettings();
            return false;
        }
        started.push_back(instance.get());
    }
    m_errorString.clear();
    setRunning(true);
    return true;
}

bool ForgeApplication::start(Component& instance)
{
    if(!std::ranges::any_of(m_components,
                            [&instance](const std::unique_ptr<Component>& owned) { return owned.get() == &instance; }))
    {
        m_errorString = QStringLiteral("%1 is not a component of the application").arg(instance.objectName());
        return false;
    }
    if(!m_running)
    {
        m_errorString =
            QStringLiteral("%1 cannot be started while the application does not run").arg(instance.objectName());
        return false;
    }
    if(instance.state() == ComponentState::Initializing && !instance.initialize())
    {
        m_errorString = QStringLiteral("%1 could not be initialized").arg(refusalIn(instance));
        return false;
    }
    if(instance.state() == ComponentState::Ready && !instance.start())
    {
        m_errorString = QStringLiteral("%1 could not be started").arg(refusalIn(instance));
        return false;
    }
    m_errorString.clear();
    return true;
}

void ForgeApplication::stop()
{
    for(const std::unique_ptr<Component>& instance : m_components | std::views::reverse)
    {
        instance->stop(); // Does nothing to one that does not run
    }
    // Once the components no longer run: what they changed last is written.
    if(m_running)
    {
        for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles | std::views::reverse)
        {
            file->stop();
        }
    }
    setRunning(false);
}

bool ForgeApplication::isRunning() const
{
    return m_running;
}

bool ForgeApplication::unloadPlugin(const QString& pluginId)
{
    if(m_running)
    {
        m_errorString = QStringLiteral("A plugin cannot be unloaded while the application runs");
        return false;
    }
    // None of them outlives what built it.
    const QList<Component*> instances = componentsOf(pluginId);
    for(Component* instance : instances)
    {
        remove(*instance);
    }
    std::erase_if(m_definitions,
                  [&pluginId](const auto& definition) { return holds(*definition.second.composite, pluginId); });
    if(!m_plugins.unload(pluginId))
    {
        m_errorString = m_plugins.errorString();
        return false;
    }
    m_errorString.clear();
    return true;
}

QString ForgeApplication::errorString() const
{
    return m_errorString;
}

QString ForgeApplication::newSettingsFileName() const
{
    QString name = QStringLiteral("settings.json");
    for(int number = 2; settingsFile(name) != nullptr; ++number)
    {
        name = QStringLiteral("settings%1.json").arg(number);
    }
    return name;
}

void ForgeApplication::setRunning(bool running)
{
    if(m_running == running)
    {
        return;
    }
    m_running = running;
    emit runningChanged(running);
}

} // namespace AppForge
