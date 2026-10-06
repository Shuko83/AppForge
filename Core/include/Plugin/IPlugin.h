#pragma once

#include <QtPlugin>

namespace AppForge
{

// Root object of every plugin: appforge_add_plugin generates it, plugins do not implement this interface themselves.
class IPlugin
{
  public:
    virtual ~IPlugin() = default;
};

} // namespace AppForge

// Versioned: PluginManager skips a plugin built against another version of IPlugin.
#define APPFORGE_PLUGIN_IID "Shuko83.AppForge.IPlugin/1.0"

Q_DECLARE_INTERFACE(AppForge::IPlugin, APPFORGE_PLUGIN_IID)
