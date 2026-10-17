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
#include <QMetaProperty>
#include <QSignalBlocker>
#include <algorithm>
#include <map>
#include <ranges>

#include "AppForge/Component/ComponentFactory.h"
#include "ProjectJson.h"

namespace AppForge
{

namespace
{

using ProjectJson::writableProperties;
using ProjectJson::writeProperty;

Q_LOGGING_CATEGORY(lcPlugins, "appforge.plugins")

// The version of the projects written, the only one read.
constexpr int ProjectVersion = 1;

// The names of the options addCommandLineOptions() adds.
const QString PluginDirOption = QStringLiteral("plugin-dir");
const QString PluginRootOption = QStringLiteral("plugin-root");

// entry as a project holds it: the name of its component, the key of its object when it is not the component, see
// ProjectJson::ObjectKey, its "index" only when it is not the first object of its name, and its property.
QJsonObject entryJson(const SettingsEntry& entry)
{
    QJsonObject json;
    json.insert(u"component", entry.component->objectName());
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
    if(project.value(u"version").toInt() != ProjectVersion)
    {
        error = QStringLiteral("It is not an AppForge project of version %1").arg(ProjectVersion);
        return std::nullopt;
    }
    return project;
}

QStringList ForgeApplication::load(const QJsonObject& project)
{
    clear();
    QStringList problems;

    const QJsonArray plugins = project.value(u"plugins").toArray();
    for(const QJsonValue& value : plugins)
    {
        const QJsonObject plugin = value.toObject();
        const QString pluginId = plugin.value(u"id").toString();
        // Where its file is from the plugin root; as it is when it is not relative.
        const QString filePath = QDir::cleanPath(m_pluginRoot.absoluteFilePath(plugin.value(u"file").toString()));
        // Opened from its file when it is not found already, as the ones of plugins/ are.
        if((!m_plugins.plugin(pluginId) && !m_plugins.addFile(filePath)) ||
           (!m_plugins.isLoaded(pluginId) && !m_plugins.load(pluginId)))
        {
            problems.append(QStringLiteral("The plugin %1 is not loaded: %2").arg(pluginId, m_plugins.errorString()));
        }
    }

    const QJsonArray components = project.value(u"components").toArray();
    for(const QJsonValue& value : components)
    {
        const QJsonObject json = value.toObject();
        const QString name = json.value(u"name").toString();
        const QString componentId = json.value(u"type").toString();
        const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
        Component* instance = info ? instantiate(componentId, name) : nullptr;
        if(instance == nullptr)
        {
            problems.append(QStringLiteral("%1 is missing: no loaded plugin provides %2").arg(name, componentId));
            continue;
        }
        const QJsonObject properties = json.value(u"properties").toObject();
        for(auto property = properties.begin(); property != properties.end(); ++property)
        {
            if(!writeProperty(*instance, info->properties, property.key(), property.value()))
            {
                problems.append(QStringLiteral("The property %1 of %2 could not be set").arg(property.key(), name));
            }
        }

        // Each to the first object of its name that none was given yet: those of a name are held in the order they
        // were added.
        QList<PropertyObject> remaining = instance->propertyObjects();
        const QJsonArray objects = json.value(u"objects").toArray();
        for(const QJsonValue& objectValue : objects)
        {
            const QJsonObject objectJson = objectValue.toObject();
            const QString objectName = objectJson.value(u"name").toString();
            const auto found = std::ranges::find(remaining, objectName, &PropertyObject::name);
            if(found == remaining.end())
            {
                problems.append(QStringLiteral("The object %1 of %2 is missing").arg(objectName, name));
                continue;
            }
            const PropertyObject restored = *found;
            remaining.erase(found);
            const QJsonObject objectProperties = objectJson.value(u"properties").toObject();
            for(auto property = objectProperties.begin(); property != objectProperties.end(); ++property)
            {
                if(!writeProperty(*restored.object, restored.properties(), property.key(), property.value()))
                {
                    problems.append(QStringLiteral("The property %1 of the object %2 of %3 could not be set")
                                        .arg(property.key(), objectName, name));
                }
            }
        }
    }

    const QJsonArray bindings = project.value(u"bindings").toArray();
    for(const QJsonValue& value : bindings)
    {
        const QJsonObject json = value.toObject();
        const QString consumerName = json.value(u"consumer").toString();
        const QString consumedName = json.value(u"consumed").toString();
        const QString providerName = json.value(u"provider").toString();
        const QString providedName = json.value(u"provided").toString();
        Component* consumer = component(consumerName);
        Component* provider = component(providerName);
        if(consumer == nullptr || provider == nullptr ||
           !consumer->bindInterface(consumedName, *provider, providedName))
        {
            problems.append(QStringLiteral("%1 of %2 could not be bound to %3 of %4")
                                .arg(consumedName, consumerName, providedName, providerName));
        }
    }

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
    const QStringList problems = load(*project);
    m_errorString = problems.join(u'\n');
    return problems.isEmpty();
}

QJsonObject ForgeApplication::save() const
{
    std::map<QString, QString> pluginFiles; // By plugin id: each of them once
    QJsonArray components;
    QJsonArray bindings;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        const std::optional<ComponentInfo> info = ComponentFactory::instance().component(instance->componentId());
        const std::optional<PluginInfo> plugin = info ? m_plugins.plugin(info->pluginId) : std::nullopt;
        if(plugin)
        {
            pluginFiles.emplace(plugin->id, plugin->filePath);
        }

        QJsonObject json;
        json.insert(u"name", instance->objectName());
        json.insert(u"type", instance->componentId());
        json.insert(u"properties", savedValues(*instance, info ? info->properties : QList<QMetaProperty>()));
        QJsonArray objects;
        const QList<PropertyObject> propertyObjects = instance->propertyObjects();
        for(const PropertyObject& added : propertyObjects)
        {
            QJsonObject object;
            object.insert(u"name", added.name);
            object.insert(u"properties", savedValues(*added.object, added.properties()));
            objects.append(object);
        }
        if(!objects.isEmpty())
        {
            json.insert(u"objects", objects);
        }
        components.append(json);

        const QList<InterfaceInfo> consumedInterfaces = instance->consumedInterfaces();
        for(const InterfaceInfo& consumed : consumedInterfaces)
        {
            const InterfaceBinding bound = instance->binding(consumed.name);
            if(bound.provider != nullptr)
            {
                QJsonObject binding;
                binding.insert(u"consumer", instance->objectName());
                binding.insert(u"consumed", consumed.name);
                binding.insert(u"provider", bound.provider->objectName());
                binding.insert(u"provided", bound.providedName);
                bindings.append(binding);
            }
        }
    }

    // The plugins of its components: those to load before it is read again.
    QJsonArray plugins;
    for(const auto& [pluginId, filePath] : pluginFiles)
    {
        QJsonObject plugin;
        plugin.insert(u"id", pluginId);
        // Relative, for the project to be read wherever its plugins are moved to, with their root; it cannot be when
        // the file is on another drive than the root.
        plugin.insert(u"file", m_pluginRoot.relativeFilePath(filePath));
        plugins.append(plugin);
    }

    QJsonArray settings;
    for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles)
    {
        QJsonObject json;
        json.insert(u"file", file->fileName());
        json.insert(u"directory", file->directory());
        QJsonArray entries;
        for(const SettingsEntry& entry : file->entries())
        {
            entries.append(entryJson(entry));
        }
        json.insert(u"entries", entries);
        settings.append(json);
    }

    QJsonObject project;
    project.insert(u"version", ProjectVersion);
    project.insert(u"plugins", plugins);
    project.insert(u"components", components);
    project.insert(u"bindings", bindings);
    if(!settings.isEmpty()) // Only for those who use them: the others are written as they were
    {
        project.insert(u"settings", settings);
    }
    return project;
}

Component* ForgeApplication::instantiate(const QString& componentId, const QString& name)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
    if(m_running)
    {
        return nullptr;
    }
    std::unique_ptr<Component> instance = ComponentFactory::instance().create(componentId);
    if(!info || instance == nullptr)
    {
        return nullptr;
    }
    instance->setObjectName(name.isEmpty() || contains(name) ? newName(info->name) : name);
    return m_components.emplace_back(std::move(instance)).get();
}

bool ForgeApplication::rename(Component& instance, const QString& name)
{
    const Component* named = component(name);
    if(m_running || name.isEmpty() || (named != nullptr && named != &instance))
    {
        return false;
    }
    instance.setObjectName(name);
    return true;
}

bool ForgeApplication::remove(Component& instance)
{
    const auto found = std::ranges::find_if(m_components, [&instance](const std::unique_ptr<Component>& owned)
                                            { return owned.get() == &instance; });
    if(m_running || found == m_components.end())
    {
        return false;
    }
    m_components.erase(found);
    return true;
}

const std::vector<std::unique_ptr<Component>>& ForgeApplication::components() const
{
    return m_components;
}

Component* ForgeApplication::component(const QString& name) const
{
    const auto found = std::ranges::find(m_components, name, &Component::objectName);
    return found != m_components.end() ? found->get() : nullptr;
}

QList<Component*> ForgeApplication::componentsOf(const QString& pluginId) const
{
    QList<Component*> found;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        const std::optional<ComponentInfo> info = ComponentFactory::instance().component(instance->componentId());
        if(info && info->pluginId == pluginId)
        {
            found.append(instance.get());
        }
    }
    return found;
}

void ForgeApplication::clear()
{
    stop();
    m_components.clear();
    if(!m_settingsFiles.empty())
    {
        m_settingsFiles.clear();
        emit settingsFilesChanged();
    }
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
    if(m_running)
    {
        m_errorString.clear();
        return true;
    }
    // Before the settings files give the properties the values of the user.
    keepEditedValues();
    for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles)
    {
        file->start();
    }
    std::vector<Component*> started;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(!instance->start())
        {
            m_errorString = QStringLiteral("%1 could not be started").arg(instance->objectName());
            for(Component* running : started | std::views::reverse)
            {
                running->stop();
            }
            returnToEdition();
            return false;
        }
        started.push_back(instance.get());
    }
    m_errorString.clear();
    setRunning(true);
    return true;
}

void ForgeApplication::stop()
{
    for(const std::unique_ptr<Component>& instance : m_components | std::views::reverse)
    {
        instance->stop(); // Does nothing to one that does not run
    }
    if(m_running)
    {
        returnToEdition();
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

QJsonObject ForgeApplication::savedValues(const QObject& object, const QList<QMetaProperty>& properties) const
{
    const auto edited = std::ranges::find(m_editedValues, &object,
                                          [](const EditedValues& values) -> const QObject* { return values.object; });
    return edited != m_editedValues.end() ? edited->values : writableProperties(object, properties);
}

void ForgeApplication::keepEditedValues()
{
    m_editedValues.clear();
    const auto keep = [this](QObject& object, const QList<QMetaProperty>& properties)
    {
        m_editedValues.push_back(EditedValues{
            .object = &object, .properties = properties, .values = writableProperties(object, properties)});
    };
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        const std::optional<ComponentInfo> info = ComponentFactory::instance().component(instance->componentId());
        keep(*instance, info ? info->properties : QList<QMetaProperty>());
        const QList<PropertyObject> objects = instance->propertyObjects();
        for(const PropertyObject& added : objects)
        {
            keep(*added.object, added.properties());
        }
    }
}

void ForgeApplication::returnToEdition()
{
    for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles | std::views::reverse)
    {
        file->stop();
    }
    // Once the files wrote what the user changed: it is undone here.
    for(const EditedValues& edited : m_editedValues)
    {
        if(edited.object == nullptr)
        {
            continue; // Destroyed while it ran
        }
        for(auto value = edited.values.begin(); value != edited.values.end(); ++value)
        {
            writeProperty(*edited.object, edited.properties, value.key(), value.value());
        }
    }
    m_editedValues.clear();
}

bool ForgeApplication::contains(const QString& name) const
{
    return component(name) != nullptr;
}

QString ForgeApplication::newName(const QString& componentName) const
{
    for(int number = 1;; ++number)
    {
        const QString name = componentName + QString::number(number);
        if(!contains(name))
        {
            return name;
        }
    }
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
