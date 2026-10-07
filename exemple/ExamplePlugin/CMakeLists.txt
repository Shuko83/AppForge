# Its root object and its metadata are generated.
# Its class (APPFORGE_PLUGIN) and its components, each registered in it with APPFORGE_REGISTER_COMPONENT, are in src/.
appforge_add_plugin(
    NAME ExamplePlugin
    DESCRIPTION "Example plugin: its metadata is read without loading it"
    VERSION 1.0.0
)
