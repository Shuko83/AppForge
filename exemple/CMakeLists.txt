# Interfaces the example components provide and consume, outside the plugins.
add_subdirectory(ExampleInterfaces)

# A plugin and an application that lists the plugins next to it.
add_subdirectory(ExamplePlugin)
add_subdirectory(ExampleApp)
# A plugin elsewhere, which Forge opens with "Open plugin...".
add_subdirectory(ExternalPlugin)
# A plugin whose components provide and consume a widget, and an application binding them.
add_subdirectory(WidgetPlugin)
add_subdirectory(WidgetApp)

# The applications load the plugins without linking to them: build the plugins with the applications.
add_dependencies(ExampleApp ExamplePlugin)
add_dependencies(WidgetApp WidgetPlugin)
