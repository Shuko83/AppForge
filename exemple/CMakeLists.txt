# A plugin and an application that lists the plugins next to it.
add_subdirectory(ExamplePlugin)
add_subdirectory(ExampleApp)
# A plugin elsewhere, which Forge opens with "Open plugin...".
add_subdirectory(ExternalPlugin)

# The application loads the plugin without linking to it: build the plugin with the application.
add_dependencies(ExampleApp ExamplePlugin)
