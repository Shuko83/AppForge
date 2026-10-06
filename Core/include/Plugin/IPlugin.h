#pragma once

#include <QtPlugin>
#include <string_view>

namespace AppForge
{

// Root object of every plugin: appforge_add_plugin generates it, plugins do not implement this interface themselves.
class IPlugin
{
  public:
    virtual ~IPlugin() = default;

    // __DATE__ " " __TIME__ of the plugin, e.g. "Oct  6 2026 21:14:03".
    [[nodiscard]] virtual std::string_view buildDate() const = 0;
};

} // namespace AppForge

// Versioned: PluginManager skips a plugin built against another version of IPlugin.
#define APPFORGE_PLUGIN_IID "Shuko83.AppForge.IPlugin/1.0"

Q_DECLARE_INTERFACE(AppForge::IPlugin, APPFORGE_PLUGIN_IID)
