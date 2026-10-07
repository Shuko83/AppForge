# A component provides a widget, another one consumes it to show it: both through IWidget, from ExampleInterfaces.
# Its class (APPFORGE_PLUGIN) and its components are in src/.
appforge_add_plugin(
    NAME WidgetPlugin
    DESCRIPTION "Example plugin: a component provides a widget, another one shows it"
    VERSION 1.0.0
    LINK_LIBRARIES PRIVATE ExampleInterfaces Qt6::Widgets
)
