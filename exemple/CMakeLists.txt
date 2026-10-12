# A plugin and an application that lists the plugins next to it.
add_subdirectory(ExamplePlugin)
add_subdirectory(ExampleApp)
# A plugin elsewhere, which Forge opens with "Open plugin...".
add_subdirectory(ExternalPlugin)
# An application that runs a project saved by Forge.
add_subdirectory(ExampleForgeApp)

# The application loads the plugin without linking to it: build the plugin with the application.
add_dependencies(ExampleApp ExamplePlugin)
# Forge neither: without this, building Forge alone rebuilds Core and leaves these plugins as they were built, against
# the Core of before. Their components then crash as soon as they are created.
add_dependencies(Forge ExamplePlugin ExternalPlugin)
# Nor does the application of a project, whichever of these plugins its components are from.
add_dependencies(ExampleForgeApp ExamplePlugin ExternalPlugin)
