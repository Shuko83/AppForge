# The plugin PluginManagerTest scans and loads: its metadata is checked by the test.
# Its class (APPFORGE_PLUGIN) and its component Probe are in src/.
appforge_add_plugin(
    NAME TestPlugin
    DESCRIPTION "Plugin of the \"PluginManager\" tests"
    VERSION 2.3.4
)
