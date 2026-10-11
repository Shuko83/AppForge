# A plugin and an application that lists the plugins next to it.
add_subdirectory(ExamplePlugin)
add_subdirectory(ExampleApp)
# A plugin elsewhere, which Forge opens with "Open plugin...".
add_subdirectory(ExternalPlugin)

# The application loads the plugin without linking to it: build the plugin with the application.
add_dependencies(ExampleApp ExamplePlugin)
# Forge neither: without this, building Forge alone rebuilds Core and leaves these plugins as they were built, against
# the Core of before. Their components then crash as soon as they are created.
add_dependencies(Forge ExamplePlugin ExternalPlugin)
