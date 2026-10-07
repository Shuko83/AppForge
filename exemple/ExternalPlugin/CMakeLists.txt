# A plugin outside <application directory>/plugins: not found when an application starts, Forge opens it with
# "Open plugin...". Its class (APPFORGE_PLUGIN) and its components are in src/.
appforge_add_plugin(
    NAME ExternalPlugin
    DESCRIPTION "Example plugin outside the plugins directory, opened from Forge"
    VERSION 1.0.0
)

# appforge_add_plugin writes it to <application directory>/plugins, which PluginManager::scan() reads by default.
# Multi-config generators add a <config> subdirectory: <build>/external-plugins/Debug with Visual Studio.
set_target_properties(ExternalPlugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/external-plugins")
