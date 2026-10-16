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

using Components = std::vector<std::unique_ptr<Component>>;

// The version of the projects written, and of the composite files: 2 has composites. Those of a version from 1 to
// this one are read.
constexpr int ProjectVersion = 2;

// The names of the options addCommandLineOptions() adds.
const QString PluginDirOption = QStringLiteral("plugin-dir");
const QString PluginRootOption = QStringLiteral("plugin-root");

// Whether component is of the plugin pluginId, or is a composite that holds one, at any depth.
bool holdsPlugin(const Component& component, const QString& pluginId)
{
    if(const auto* composite = qobject_cast<const Composite*>(&component))
    {
        return std::ranges::any_of(composite->components(), [&pluginId](const std::unique_ptr<Component>& inner)
                                   { return holdsPlugin(*inner, pluginId); });
    }
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    return info && info->pluginId == pluginId;
}

// Whether path, of a binding among components, names the one named name, or one of its components.
bool names(const QString& path, const QString& name)
{
    return path == name || path.startsWith(name + ComponentPaths::Separator);
}

// bindings, among components, once the one named name is named newName.
QJsonArray renamed(const QJsonArray& bindings, const QString& name, const QString& newName)
{
    QJsonArray result;
    for(const QJsonValue& value : bindings)
    {
        QJsonObject binding = value.toObject();
        for(const QStringView key : {u"consumer", u"provider"})
        {
            const QString bound = binding.value(key).toString();
            if(names(bound, name))
            {
                binding.insert(key, newName + bound.sliced(name.size()));
            }
        }
        result.append(binding);
    }
    return result;
}

// bindings, among components, once the one named name is gone.
QJsonArray without(const QJsonArray& bindings, const QString& name)
{
    QJsonArray result;
    for(const QJsonValue& value : bindings)
    {
        const QJsonObject binding = value.toObject();
        if(!names(binding.value(u"consumer").toString(), name) && !names(binding.value(u"provider").toString(), name))
        {
            result.append(binding);
        }
    }
    return result;
}

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

Component* ForgeApplication::instantiate(const QString& componentId, const QString& name, Composite* in)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
    // Not one Core provides: a composite is made from its file.
    if(!info || info->pluginId == ComponentFactory::corePluginId() || (in != nullptr && !canChange(*in)))
    {
        return nullptr;
    }
    Components& components = in != nullptr ? in->m_components : m_components;
    const QString named =
        ComponentPaths::canName(components, name) ? name : ComponentPaths::newName(components, info->name);
    if(in == nullptr)
    {
        std::unique_ptr<Component> instance = ComponentFactory::instance().create(componentId);
        instance->setObjectName(named);
        return m_components.emplace_back(std::move(instance)).get();
    }

    // In each instance of its file, which have the same names.
    Component* created = nullptr;
    const QList<Instance> instances = instancesOf(in->filePath());
    for(const Instance& instance : instances)
    {
        std::unique_ptr<Component> component = ComponentFactory::instance().create(componentId);
        component->setObjectName(named);
        if(instance.composite == in)
        {
            created = component.get();
        }
        instance.composite->m_components.push_back(std::move(component));
    }
    return created;
}

Composite* ForgeApplication::instantiateComposite(const QString& filePath, const QString& name, Composite* in)
{
    const QString file = Assembler::compositeFilePath(QDir(), filePath);
    if(in != nullptr && !canChange(*in))
    {
        return nullptr;
    }
    Assembler assembler(*this);
    if(!assembler.define(file, m_errorString))
    {
        return nullptr;
    }
    if(in != nullptr && (file == in->filePath() || holdsFile(file, in->filePath())))
    {
        m_errorString = QStringLiteral("%1 cannot be placed in %2: it would contain itself")
                            .arg(QFileInfo(file).fileName(), QFileInfo(in->filePath()).fileName());
        return nullptr;
    }
    Components& components = in != nullptr ? in->m_components : m_components;
    const QString named = ComponentPaths::canName(components, name)
                              ? name
                              : ComponentPaths::newName(components, QFileInfo(file).completeBaseName());

    Composite* placed = nullptr;
    const QList<Instance> instances = in != nullptr ? instancesOf(in->filePath()) : QList<Instance>{{}};
    for(const Instance& instance : instances)
    {
        QString error;
        std::unique_ptr<Composite> composite = assembler.instantiate(file, error, instance.bound);
        if(composite == nullptr) // Its definition is read: it does not contain itself
        {
            continue;
        }
        composite->setObjectName(named);
        if(instance.composite == in)
        {
            placed = composite.get();
        }
        (instance.composite != nullptr ? instance.composite->m_components : m_components)
            .push_back(std::move(composite));
    }
    m_errorString = assembler.problems().join(u'\n');
    return placed;
}

bool ForgeApplication::rename(Component& instance, const QString& name)
{
    Composite* in = compositeOf(instance);
    if(in == nullptr)
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
    if(!canChange(*in) || !ComponentPaths::canName(in->m_components, name, &instance))
    {
        return false;
    }
    const QString former = instance.objectName();
    const QList<Instance> instances = instancesOf(in->filePath());
    for(const Instance& other : instances)
    {
        if(Component* renamed = ComponentPaths::find(other.composite->m_components, former))
        {
            renamed->setObjectName(name);
        }
    }
    QJsonArray& bindings = m_definitions.at(in->filePath()).bindings;
    bindings = renamed(bindings, former, name);
    return true;
}

void ForgeApplication::remove(Component& instance)
{
    Composite* in = compositeOf(instance);
    if(in == nullptr)
    {
        const auto found = std::ranges::find_if(m_components, [&instance](const std::unique_ptr<Component>& owned)
                                                { return owned.get() == &instance; });
        if(found == m_components.end())
        {
            return;
        }
        instance.stop(); // Does nothing to one that does not run
        m_components.erase(found);
        return;
    }
    if(!canChange(*in))
    {
        return;
    }
    const QString name = instance.objectName(); // instance is one of those that go
    const QList<Instance> instances = instancesOf(in->filePath());
    for(const Instance& other : instances)
    {
        Components& components = other.composite->m_components;
        const auto found = std::ranges::find(components, name, [](const std::unique_ptr<Component>& component)
                                             { return component->objectName(); });
        if(found != components.end())
        {
            (*found)->stop();
            components.erase(found);
        }
    }
    QJsonArray& bindings = m_definitions.at(in->filePath()).bindings;
    bindings = without(bindings, name);
}

bool ForgeApplication::bind(Component& consumer, const QString& consumed, Component& provider, const QString& provided)
{
    Composite* in = compositeOf(consumer);
    if(in != compositeOf(provider) || (in == nullptr && (path(consumer).isEmpty() || path(provider).isEmpty())))
    {
        m_errorString = QStringLiteral("%1 and %2 are not of the same composite").arg(path(consumer), path(provider));
        return false;
    }
    if(in == nullptr)
    {
        return consumer.bindInterface(consumed, provider, provided);
    }
    // Asked of the one given first: the others are alike.
    if(!canChange(*in) || !consumer.bindInterface(consumed, provider, provided))
    {
        return false;
    }
    const QString consumerName = consumer.objectName();
    const QString providerName = provider.objectName();
    const QList<Instance> instances = instancesOf(in->filePath());
    for(const Instance& other : instances)
    {
        Component* otherConsumer = ComponentPaths::find(other.composite->m_components, consumerName);
        Component* otherProvider = ComponentPaths::find(other.composite->m_components, providerName);
        if(other.bound && other.composite != in && otherConsumer != nullptr && otherProvider != nullptr)
        {
            otherConsumer->bindInterface(consumed, *otherProvider, provided);
        }
    }
    // In place of the one it had: what is consumed is bound to one interface.
    QJsonArray& bindings = m_definitions.at(in->filePath()).bindings;
    for(qsizetype index = bindings.size() - 1; index >= 0; --index)
    {
        const QJsonObject binding = bindings.at(index).toObject();
        if(binding.value(u"consumer").toString() == consumerName && binding.value(u"consumed").toString() == consumed)
        {
            bindings.removeAt(index);
        }
    }
    QJsonObject binding;
    binding.insert(u"consumer", consumerName);
    binding.insert(u"consumed", consumed);
    binding.insert(u"provider", providerName);
    binding.insert(u"provided", provided);
    bindings.append(binding);
    return true;
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

Composite* ForgeApplication::compositeOf(const Component& component) const
{
    const QString found = path(component);
    const qsizetype separator = found.lastIndexOf(ComponentPaths::Separator);
    return separator < 0 ? nullptr : qobject_cast<Composite*>(this->component(QStringView(found).first(separator)));
}

bool ForgeApplication::loadComposite(const QString& filePath)
{
    Assembler assembler(*this);
    if(!assembler.define(Assembler::compositeFilePath(QDir(), filePath), m_errorString))
    {
        return false;
    }
    m_errorString = assembler.problems().join(u'\n');
    return true;
}

QStringList ForgeApplication::compositeFiles() const
{
    QStringList files;
    for(const QString& file : m_definitions | std::views::keys)
    {
        files.append(file);
    }
    return files;
}

QJsonObject ForgeApplication::saveComposite(const QString& filePath) const
{
    if(!m_definitions.contains(filePath))
    {
        return {};
    }
    QJsonObject document = Assembler::document(*this, filePath);
    document.insert(u"version", ProjectVersion);
    return document;
}

bool ForgeApplication::isComplete(const QString& filePath) const
{
    const auto found = m_definitions.find(filePath);
    return found != m_definitions.end() && found->second.complete;
}

std::optional<QJsonObject> ForgeApplication::compose(const QList<Component*>& components, const QString& directory)
{
    for(const Component* component : components)
    {
        if(!std::ranges::any_of(m_components, [component](const std::unique_ptr<Component>& owned)
                                { return owned.get() == component; }))
        {
            m_errorString = QStringLiteral("%1 is not a component of the application").arg(component->objectName());
            return std::nullopt;
        }
    }
    // In the order they were instantiated, as the application holds them.
    QList<const Component*> composed;
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(components.contains(component.get()))
        {
            composed.append(component.get());
        }
    }
    // A binding of one of them to one that is not, either way, would be lost.
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        const bool inside = components.contains(instance.get());
        const QList<InterfaceInfo> consumedInterfaces = instance->consumedInterfaces();
        for(const InterfaceInfo& consumed : consumedInterfaces)
        {
            const InterfaceBinding bound = instance->binding(consumed.name);
            if(bound.provider == nullptr)
            {
                continue;
            }
            // The one of the application it is, or that holds it.
            const QString providerPath = path(*bound.provider);
            Component* top = component(providerPath.section(ComponentPaths::Separator, 0, 0));
            if(inside != components.contains(top))
            {
                m_errorString = QStringLiteral("%1 of %2 is bound to %3 of %4: a composite cannot hold one without the "
                                               "other")
                                    .arg(consumed.name, instance->objectName(), bound.providedName, providerPath);
                return std::nullopt;
            }
        }
    }
    QJsonObject document = Assembler::save(composed, directory, m_plugins, m_pluginRoot);
    document.insert(u"version", ProjectVersion);
    return document;
}

Composite* ForgeApplication::group(const QList<Component*>& components, const QString& filePath)
{
    // The properties of theirs settings files hold: the same, of its components, once it replaces them.
    struct Held
    {
        SettingsFile* file;
        QString component; // Its path from the composite: the one it has from the application
        std::optional<ProjectJson::ObjectKey> object;
        QString property;
    };
    QList<Held> held;
    for(const std::unique_ptr<SettingsFile>& file : m_settingsFiles)
    {
        for(const SettingsEntry& entry : file->entries())
        {
            const QString entryPath = path(*entry.component);
            const QString top = entryPath.section(ComponentPaths::Separator, 0, 0);
            if(std::ranges::any_of(components,
                                   [&top](const Component* grouped) { return grouped->objectName() == top; }))
            {
                held.append(Held{.file = file.get(),
                                 .component = entryPath,
                                 .object = ProjectJson::objectKey(*entry.component, entry.object),
                                 .property = entry.property});
            }
        }
    }

    Composite* placed = instantiateComposite(filePath);
    if(placed == nullptr)
    {
        return nullptr;
    }
    const QString problems = m_errorString;
    for(Component* grouped : components)
    {
        remove(*grouped);
    }
    for(const Held& entry : std::as_const(held))
    {
        Component* component = placed->component(entry.component);
        QObject* object =
            component != nullptr && entry.object ? ProjectJson::object(*component, *entry.object) : component;
        if(object != nullptr)
        {
            entry.file->add({.component = component, .object = object, .property = entry.property});
        }
    }
    m_errorString = problems;
    return placed;
}

QList<Component*> ForgeApplication::componentsOf(const QString& pluginId) const
{
    QList<Component*> found;
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(holdsPlugin(*instance, pluginId))
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

    std::vector<Component*> started;
    if(!startAll(m_components, started))
    {
        for(Component* running : started | std::views::reverse)
        {
            running->stop();
        }
        stopSettings();
        return false;
    }
    m_errorString.clear();
    setRunning(true);
    return true;
}

bool ForgeApplication::start(Component& instance)
{
    if(path(instance).isEmpty())
    {
        m_errorString = QStringLiteral("%1 is not a component of the application").arg(instance.objectName());
        return false;
    }
    if(!m_running)
    {
        m_errorString = QStringLiteral("%1 cannot be started while the application does not run").arg(path(instance));
        return false;
    }
    if(const Composite* in = compositeOf(instance); in != nullptr && in->state() != ComponentState::Running)
    {
        m_errorString = QStringLiteral("%1 cannot be started while %2 does not run").arg(path(instance), path(*in));
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
                  [&pluginId](const auto& definition) { return holdsPlugin(*definition.second.composite, pluginId); });
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

QList<ForgeApplication::Instance> ForgeApplication::instancesOf(const QString& filePath) const
{
    QList<Instance> instances;
    // Through every composite: one of filePath can be in any of them, but in itself.
    const auto collect = [&filePath, &instances](const auto& self, const Components& components, bool bound) -> void
    {
        for(const std::unique_ptr<Component>& component : components)
        {
            if(auto* composite = qobject_cast<Composite*>(component.get()))
            {
                if(composite->filePath() == filePath)
                {
                    instances.append(Instance{.composite = composite, .bound = bound});
                }
                self(self, composite->components(), bound);
            }
        }
    };
    collect(collect, m_components, true);
    for(const auto& [file, definition] : m_definitions)
    {
        if(file == filePath)
        {
            instances.append(Instance{.composite = definition.composite.get(), .bound = false});
        }
        collect(collect, definition.composite->components(), false);
    }
    return instances;
}

bool ForgeApplication::canChange(const Composite& composite)
{
    if(!isComplete(composite.filePath()))
    {
        m_errorString = QStringLiteral("%1 could not be entirely read: it is not changed, which would lose what is "
                                       "missing")
                            .arg(QFileInfo(composite.filePath()).fileName());
        return false;
    }
    return true;
}

bool ForgeApplication::holdsFile(const QString& filePath, const QString& held) const
{
    const auto definition = m_definitions.find(filePath);
    if(definition == m_definitions.end())
    {
        return false;
    }
    const auto holding = [&held](const auto& self, const Components& components) -> bool
    {
        return std::ranges::any_of(components,
                                   [&self, &held](const std::unique_ptr<Component>& component)
                                   {
                                       const auto* composite = qobject_cast<const Composite*>(component.get());
                                       return composite != nullptr &&
                                              (composite->filePath() == held || self(self, composite->components()));
                                   });
    };
    return holding(holding, definition->second.composite->components());
}

bool ForgeApplication::startAll(const Components& components, std::vector<Component*>& started)
{
    // All of them before any is started: one that starts can count on the others being ready.
    for(const std::unique_ptr<Component>& instance : components)
    {
        if(instance->state() == ComponentState::Initializing && !instance->initialize())
        {
            m_errorString = QStringLiteral("%1 could not be initialized").arg(refusalIn(*instance));
            return false;
        }
    }
    for(const std::unique_ptr<Component>& instance : components)
    {
        if(instance->state() == ComponentState::Running)
        {
            // What was added to it meanwhile.
            const auto* composite = qobject_cast<const Composite*>(instance.get());
            if(composite != nullptr && !startAll(composite->components(), started))
            {
                return false;
            }
            continue;
        }
        if(instance->state() != ComponentState::Ready)
        {
            continue;
        }
        if(!instance->start())
        {
            m_errorString = QStringLiteral("%1 could not be started").arg(refusalIn(*instance));
            return false;
        }
        started.push_back(instance.get());
    }
    return true;
}

QString ForgeApplication::refusalIn(const Component& refused) const
{
    const auto* composite = qobject_cast<const Composite*>(&refused);
    return composite != nullptr ? path(refused) + ComponentPaths::Separator + composite->refusal() : path(refused);
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
