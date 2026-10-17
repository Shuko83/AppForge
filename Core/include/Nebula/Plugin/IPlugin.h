// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QtPlugin>
#include <string_view>

namespace Nebula
{

class Plugin;

// Root object of every plugin: nebula_add_plugin generates it, plugins do not implement this interface themselves.
class IPlugin
{
  public:
    virtual ~IPlugin() = default;

    // __DATE__ " " __TIME__ of the plugin, e.g. "Oct  6 2026 21:14:03".
    [[nodiscard]] virtual std::string_view buildDate() const = 0;

    // The plugin class declared with NEBULA_PLUGIN, which builds the components of the plugin.
    [[nodiscard]] virtual Plugin& plugin() = 0;
};

} // namespace Nebula

// Versioned: PluginManager skips a plugin built against another version of IPlugin.
#define NEBULA_PLUGIN_IID "FabienDG.Nebula.IPlugin/2.0"

Q_DECLARE_INTERFACE(Nebula::IPlugin, NEBULA_PLUGIN_IID)
