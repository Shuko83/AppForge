#pragma once

#include <QList>
#include <QString>
#include <map>
#include <memory>
#include <optional>

#include "Component/Component.h"
#include "Component/ComponentInfo.h"
#include "Core_export.h"
#include "Plugin/Plugin.h"

namespace AppForge
{

// Knows the components of the loaded plugins and is the only way to instantiate them.
class CORE_EXPORT ComponentFactory
{
  public:
    ComponentFactory();
    ~ComponentFactory();

    ComponentFactory(const ComponentFactory&) = delete;
    ComponentFactory& operator=(const ComponentFactory&) = delete;

    // The factory of the application, where PluginManager::load() registers the components of a plugin.
    static ComponentFactory& instance();

    // Registers the components of plugin, whose id is pluginId; does nothing when they are already registered.
    // Skips, with a warning, a component whose id is already registered.
    void add(const QString& pluginId, Plugin& plugin);

    [[nodiscard]] QList<ComponentInfo> components() const;
    [[nodiscard]] std::optional<ComponentInfo> component(const QString& componentId) const;

    // Has its plugin build a component, in the Initializing state; nullptr when componentId is not registered.
    [[nodiscard]] std::unique_ptr<Component> create(const QString& componentId) const;

  private:
    struct Type
    {
        ComponentInfo info;
        Plugin* plugin = nullptr; // Never unloaded, see PluginManager
        Plugin::Builder builder = nullptr;
    };

    std::map<QString, Type> m_types; // By id
};

} // namespace AppForge
