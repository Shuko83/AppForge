// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "AppForge/Application/ForgeApplication.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaProperty>
#include <QSignalBlocker>
#include <algorithm>
#include <map>
#include <ranges>

#include "AppForge/Component/ComponentFactory.h"

namespace AppForge
{

namespace
{

// The version of the projects written, the only one read.
constexpr int ProjectVersion = 1;

// value as a project holds it; nullopt when it would not be read back as it is, its type being one JSON has nothing
// for.
std::optional<QJsonValue> toJson(const QVariant& value)
{
    const QJsonValue json = QJsonValue::fromVariant(value);
    QVariant read = json.toVariant();
    if(!read.convert(value.metaType()) || read != value)
    {
        return std::nullopt;
    }
    return json;
}

// The properties of object a project holds, by name: those of properties that can be written.
QJsonObject writableProperties(const QObject& object, const QList<QMetaProperty>& properties)
{
    QJsonObject json;
    for(const QMetaProperty& property : properties)
    {
        const std::optional<QJsonValue> value = property.isWritable() ? toJson(property.read(&object)) : std::nullopt;
        if(value)
        {
            json.insert(QLatin1StringView(property.name()), *value);
        }
    }
    return json;
}

// Sets the property name of object, one of properties, to value; false when it is none of them or cannot be set.
bool writeProperty(QObject& object, const QList<QMetaProperty>& properties, const QString& name,
                   const QJsonValue& value)
{
    const auto property = std::ranges::find(properties, name, [](const QMetaProperty& known)
                                            { return QString::fromLatin1(known.name()); });
    return property != properties.end() && property->write(&object, value.toVariant());
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
        json.insert(u"properties", writableProperties(*instance, info ? info->properties : QList<QMetaProperty>()));
        QJsonArray objects;
        const QList<PropertyObject> propertyObjects = instance->propertyObjects();
        for(const PropertyObject& added : propertyObjects)
        {
            QJsonObject object;
            object.insert(u"name", added.name);
            object.insert(u"properties", writableProperties(*added.object, added.properties()));
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

    QJsonObject project;
    project.insert(u"version", ProjectVersion);
    project.insert(u"plugins", plugins);
    project.insert(u"components", components);
    project.insert(u"bindings", bindings);
    return project;
}

Component* ForgeApplication::instantiate(const QString& componentId, const QString& name)
{
    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
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
    if(name.isEmpty() || (named != nullptr && named != &instance))
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
}

bool ForgeApplication::start()
{
    // All of them before any is started: one that starts can count on the others being ready.
    for(const std::unique_ptr<Component>& instance : m_components)
    {
        if(instance->state() == ComponentState::Initializing && !instance->initialize())
        {
            m_errorString = QStringLiteral("%1 could not be initialized").arg(instance->objectName());
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
            m_errorString = QStringLiteral("%1 could not be started").arg(instance->objectName());
            for(Component* running : started | std::views::reverse)
            {
                running->stop();
            }
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
    if(instance.state() == ComponentState::Initializing && !instance.initialize())
    {
        m_errorString = QStringLiteral("%1 could not be initialized").arg(instance.objectName());
        return false;
    }
    if(instance.state() == ComponentState::Ready && !instance.start())
    {
        m_errorString = QStringLiteral("%1 could not be started").arg(instance.objectName());
        return false;
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
