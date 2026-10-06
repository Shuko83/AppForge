# A plugin needs nothing else: its root object and its metadata are generated.
# Its sources go in src/, like for any cmu_add_target target.
appforge_add_plugin(
    NAME ExamplePlugin
    DESCRIPTION "Example plugin: its metadata is read without loading it"
    VERSION 1.0.0
)
