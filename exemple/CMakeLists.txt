# A plugin and an application that lists the plugins next to it.
add_subdirectory(ExamplePlugin)
add_subdirectory(ExampleApp)

# The application loads the plugin without linking to it: build the plugin with the application.
add_dependencies(ExampleApp ExamplePlugin)
