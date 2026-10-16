// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QString>
#include <map>
#include <memory>
#include <optional>

#include "AppForge/Component/Component.h"
#include "AppForge/Component/ComponentInfo.h"
#include "AppForge/Plugin/Plugin.h"
#include "Core_export.h"

namespace AppForge
{

// Knows the components of the loaded plugins and is the only way to instantiate them. It also knows those Core provides
// itself, as a plugin would, under corePluginId(): Composite.
class CORE_EXPORT ComponentFactory
{
  public:
    // Registers the components of Core.
    ComponentFactory();
    ~ComponentFactory();

    ComponentFactory(const ComponentFactory&) = delete;
    ComponentFactory& operator=(const ComponentFactory&) = delete;

    // The factory of the application, where PluginManager::load() registers the components of a plugin.
    static ComponentFactory& instance();
    // The plugin id of the components Core provides, built as appforge_add_plugin builds the id of a plugin:
    // FabienDG.AppForge.Core. No PluginManager knows it.
    [[nodiscard]] static QString corePluginId();

    // Registers the components of plugin, whose id is pluginId; does nothing when they are already registered.
    // Skips, with a warning, a component whose id is already registered.
    void add(const QString& pluginId, Plugin& plugin);
    // Forgets the components of the plugin pluginId, as PluginManager::unload() does before the plugin goes: none of
    // them can be instantiated any longer, and those that were must not outlive it.
    void remove(const QString& pluginId);

    [[nodiscard]] QList<ComponentInfo> components() const;
    [[nodiscard]] std::optional<ComponentInfo> component(const QString& componentId) const;

    // Has its plugin build a component, in the Initializing state; nullptr when componentId is not registered.
    [[nodiscard]] std::unique_ptr<Component> create(const QString& componentId) const;

  private:
    struct Type
    {
        ComponentInfo info;
        Plugin* plugin = nullptr; // Removed before it is destroyed, see PluginManager::unload()
        Plugin::Builder builder = nullptr;
    };

    std::unique_ptr<Plugin> m_corePlugin; // Builds the components of Core; destroyed after m_types
    std::map<QString, Type> m_types;      // By id
};

} // namespace AppForge
