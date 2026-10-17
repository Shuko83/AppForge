# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# A plugin outside <application directory>/plugins: not found when an application starts, NebulaStudio opens it with
# "Open plugin...". Its class (NEBULA_PLUGIN) and its components are in src/.
# Empty: not installed in plugins/ next to the applications either.
set(cmu_plugins_dir "")
nebula_add_plugin(
    NAME ExternalPlugin
    DESCRIPTION "Example plugin outside the plugins directory, opened from NebulaStudio"
    VERSION 1.0.0
    # Its Ticker provides a widget.
    LINK_LIBRARIES PRIVATE Qt6::Widgets
)

# nebula_add_plugin writes it to <application directory>/plugins, which PluginManager::scan() reads by default.
# Multi-config generators add a <config> subdirectory: <build>/external-plugins/Debug with Visual Studio.
set_target_properties(ExternalPlugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/external-plugins")
