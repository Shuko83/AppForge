// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Assembler.h"

#include <QFileInfo>
#include <QJsonArray>
#include <map>
#include <optional>
#include <utility>

#include "AppForge/Application/ForgeApplication.h"
#include "AppForge/Component/ComponentFactory.h"
#include "AppForge/Composite/Composite.h"
#include "AppForge/Plugin/PluginManager.h"
#include "Composite/ComponentPaths.h"
#include "ProjectJson.h"

namespace AppForge
{

namespace
{

// The id ComponentFactory registers Composite under, as it registers the component of a plugin: <plugin id>.<class>.
QString compositeId()
{
    return ComponentFactory::corePluginId() + QStringLiteral(".Composite");
}

// Whether Component::bindInterface() would bind consumed, which consumer consumes, to provided, which provider
// provides: they are two, and their interfaces exist with the same type.
bool canBind(const Component& consumer, const QString& consumed, const Component& provider, const QString& provided)
{
    const QList<InterfaceInfo> consumedInterfaces = consumer.consumedInterfaces();
    const QList<InterfaceInfo> providedInterfaces = provider.providedInterfaces();
    const auto in = std::ranges::find(consumedInterfaces, consumed, &InterfaceInfo::name);
    const auto out = std::ranges::find(providedInterfaces, provided, &InterfaceInfo::name);
    return &consumer != &provider && in != consumedInterfaces.end() && out != providedInterfaces.end() &&
           in->type == out->type;
}

} // namespace

Assembler::Assembler(ForgeApplication& application) : m_application(application) {}

const QStringList& Assembler::problems() const
{
    return m_problems;
}

void Assembler::loadPlugins(const QJsonObject& document)
{
    PluginManager& plugins = m_application.m_plugins;
    const QJsonArray entries = document.value(u"plugins").toArray();
    for(const QJsonValue& value : entries)
    {
        const QJsonObject plugin = value.toObject();
        const QString pluginId = plugin.value(u"id").toString();
        const QString file = plugin.value(u"file").toString();
        if(!plugins.plugin(pluginId) && file.isEmpty())
        {
            problem(QStringLiteral("The plugin %1 is not loaded: no plugin of that id is known").arg(pluginId));
            continue;
        }
        // Where its file is from the plugin root; as it is when it is not relative.
        const QString filePath = QDir::cleanPath(m_application.m_pluginRoot.absoluteFilePath(file));
        // Opened from its file when it is not found already, as the ones of plugins/ are.
        if((!plugins.plugin(pluginId) && !plugins.addFile(filePath)) ||
           (!plugins.isLoaded(pluginId) && !plugins.load(pluginId)))
        {
            problem(QStringLiteral("The plugin %1 is not loaded: %2").arg(pluginId, plugins.errorString()));
        }
    }
}

bool Assembler::load(Components& components, const QJsonObject& document, const QDir& directory, const QString& prefix,
                     bool bound)
{
    const QJsonArray entries = document.value(u"components").toArray();
    for(const QJsonValue& value : entries)
    {
        const QJsonObject json = value.toObject();
        const QString name = json.value(u"name").toString();
        std::unique_ptr<Component> component;
        QString base; // What it is named after when it cannot be named name
        if(json.contains(u"composite"))
        {
            const QString filePath = compositeFilePath(directory, json.value(u"composite").toString());
            QString error;
            component = instantiate(filePath, error, bound);
            if(component == nullptr)
            {
                // None of the files being read can be: they all hold the one that contains itself.
                if(!m_reading.isEmpty() && !m_cycle.isEmpty())
                {
                    return false;
                }
                m_cycle.clear();
                problem(QStringLiteral("%1 is missing: %2").arg(prefix + name, error));
                continue;
            }
            base = QFileInfo(filePath).completeBaseName();
        }
        else
        {
            const QString componentId = json.value(u"type").toString();
            const std::optional<ComponentInfo> info = ComponentFactory::instance().component(componentId);
            // Not one Core provides: a composite is made from its file.
            if(info && info->pluginId != ComponentFactory::corePluginId())
            {
                component = ComponentFactory::instance().create(componentId);
                base = info->name;
            }
            if(component == nullptr)
            {
                problem(QStringLiteral("%1 is missing: no loaded plugin provides %2").arg(prefix + name, componentId));
                continue;
            }
        }

        if(ComponentPaths::canName(components, name))
        {
            component->setObjectName(name);
        }
        else
        {
            component->setObjectName(ComponentPaths::newName(components, base));
            problem(QStringLiteral("%1 is named %2: its name is empty, holds a %3 or is the one of another component")
                        .arg(prefix + name, prefix + component->objectName(), ComponentPaths::Separator));
        }
        QStringList unset;
        ProjectJson::setValues(*component, json, prefix + component->objectName(), unset);
        for(const QString& why : std::as_const(unset))
        {
            problem(why);
        }
        components.push_back(std::move(component));
    }
    if(bound)
    {
        bind(components, document.value(u"bindings").toArray(), prefix, true);
    }
    return true;
}

QJsonArray Assembler::bind(const Components& components, const QJsonArray& bindings, const QString& prefix, bool bound)
{
    QJsonArray possible;
    for(const QJsonValue& value : bindings)
    {
        const QJsonObject json = value.toObject();
        const QString consumerName = json.value(u"consumer").toString();
        const QString consumedName = json.value(u"consumed").toString();
        const QString providerName = json.value(u"provider").toString();
        const QString providedName = json.value(u"provided").toString();
        Component* consumer = ComponentPaths::find(components, consumerName);
        Component* provider = ComponentPaths::find(components, providerName);
        if(consumer == nullptr || provider == nullptr ||
           !(bound ? consumer->bindInterface(consumedName, *provider, providedName)
                   : canBind(*consumer, consumedName, *provider, providedName)))
        {
            problem(QStringLiteral("%1 of %2 could not be bound to %3 of %4")
                        .arg(consumedName, prefix + consumerName, providedName, prefix + providerName));
            continue;
        }
        possible.append(json);
    }
    return possible;
}

std::unique_ptr<Composite> Assembler::instantiate(const QString& filePath, QString& error, bool bound)
{
    if(!define(filePath, error))
    {
        return nullptr;
    }
    // From its definition rather than from its file: its instances share the structure of the definition.
    const QString context = std::exchange(m_context, QFileInfo(filePath).fileName() + QStringLiteral(": "));
    std::unique_ptr<Composite> composite = build(filePath, document(m_application, filePath), error, bound);
    m_context = context;
    return composite;
}

QJsonObject Assembler::document(const ForgeApplication& application, const QString& filePath)
{
    const ForgeApplication::Definition& definition = application.m_definitions.at(filePath);
    QJsonObject document = save(definition.composite->components(), QFileInfo(filePath).absolutePath(),
                                application.m_plugins, application.m_pluginRoot);
    document.insert(u"bindings", definition.bindings); // Its components are bound to nothing
    return document;
}

QJsonObject Assembler::save(const Components& components, const QString& directory, const PluginManager& plugins,
                            const QDir& pluginRoot)
{
    std::map<QString, QString> pluginFiles; // By plugin id: each of them once
    QJsonArray entries;
    QJsonArray bindings;
    for(const std::unique_ptr<Component>& component : components)
    {
        QJsonObject json = ProjectJson::values(*component);
        json.insert(u"name", component->objectName());
        if(const auto* composite = qobject_cast<const Composite*>(component.get()))
        {
            // Relative, for the project to be read wherever it is moved to with its composite files.
            json.insert(u"composite", directory.isEmpty() ? composite->filePath()
                                                          : QDir(directory).relativeFilePath(composite->filePath()));
        }
        else
        {
            json.insert(u"type", component->componentId());
            const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component->componentId());
            if(const std::optional<PluginInfo> plugin = info ? plugins.plugin(info->pluginId) : std::nullopt)
            {
                pluginFiles.emplace(plugin->id, plugin->filePath);
            }
        }
        entries.append(json);

        const QList<InterfaceInfo> consumedInterfaces = component->consumedInterfaces();
        for(const InterfaceInfo& consumed : consumedInterfaces)
        {
            const InterfaceBinding bound = component->binding(consumed.name);
            // Only to one of them: a binding to anything else could not be restored.
            const QString provider =
                bound.provider != nullptr ? ComponentPaths::path(components, *bound.provider) : QString();
            if(!provider.isEmpty())
            {
                QJsonObject binding;
                binding.insert(u"consumer", component->objectName());
                binding.insert(u"consumed", consumed.name);
                binding.insert(u"provider", provider);
                binding.insert(u"provided", bound.providedName);
                bindings.append(binding);
            }
        }
    }

    // The plugins of the components: those to load before they are read again.
    QJsonArray pluginEntries;
    for(const auto& [pluginId, filePath] : pluginFiles)
    {
        QJsonObject plugin;
        plugin.insert(u"id", pluginId);
        // Relative, for the project to be read wherever its plugins are moved to, with their root; it cannot be when
        // the file is on another drive than the root.
        plugin.insert(u"file", pluginRoot.relativeFilePath(filePath));
        pluginEntries.append(plugin);
    }

    QJsonObject document;
    document.insert(u"plugins", pluginEntries);
    document.insert(u"components", entries);
    document.insert(u"bindings", bindings);
    return document;
}

QString Assembler::compositeFilePath(const QDir& directory, const QString& fileName)
{
    const QFileInfo file(directory.absoluteFilePath(fileName));
    // The same file however it is written: its definition is read once.
    const QString canonical = file.canonicalFilePath();
    return canonical.isEmpty() ? QDir::cleanPath(file.absoluteFilePath()) : canonical;
}

bool Assembler::define(const QString& filePath, QString& error)
{
    std::map<QString, ForgeApplication::Definition>& definitions = m_application.m_definitions;
    if(definitions.contains(filePath))
    {
        return true;
    }
    const QString fileName = QFileInfo(filePath).fileName();
    if(const qsizetype reading = m_reading.indexOf(filePath); reading >= 0)
    {
        QStringList chain;
        for(const QString& held : m_reading.sliced(reading))
        {
            chain.append(QFileInfo(held).fileName());
        }
        chain.append(fileName);
        m_cycle = QStringLiteral("%1 contains itself: %2").arg(fileName, chain.join(QStringLiteral(" > ")));
        error = m_cycle;
        return false;
    }
    QString unread;
    const std::optional<QJsonObject> document = ForgeApplication::read(filePath, unread);
    if(!document)
    {
        error = QStringLiteral("%1 cannot be read: %2").arg(QDir::toNativeSeparators(filePath), unread);
        return false;
    }

    m_reading.append(filePath);
    const QString context = std::exchange(m_context, fileName + QStringLiteral(": "));
    const qsizetype problemCount = m_problems.size();
    std::unique_ptr<Composite> composite = build(filePath, *document, error, false);
    // Those that can be, told once rather than each time an instance is placed.
    const QJsonArray bindings = composite != nullptr
                                    ? bind(composite->components(), document->value(u"bindings").toArray(), {}, false)
                                    : QJsonArray();
    const bool complete = m_problems.size() == problemCount;
    m_context = context;
    m_reading.removeLast();
    if(composite == nullptr)
    {
        return false;
    }
    definitions.emplace(filePath, ForgeApplication::Definition{
                                      .composite = std::move(composite), .bindings = bindings, .complete = complete});
    return true;
}

std::unique_ptr<Composite> Assembler::build(const QString& filePath, const QJsonObject& document, QString& error,
                                            bool bound)
{
    // Created as a component of a plugin is: only ComponentFactory can.
    std::unique_ptr<Composite> composite(
        static_cast<Composite*>(ComponentFactory::instance().create(compositeId()).release()));
    composite->m_filePath = filePath;
    loadPlugins(document);
    if(!load(composite->m_components, document, QFileInfo(filePath).absoluteDir(), {}, bound))
    {
        error = m_cycle;
        return nullptr;
    }
    return composite;
}

void Assembler::problem(const QString& why)
{
    m_problems.append(m_context + why);
}

} // namespace AppForge
