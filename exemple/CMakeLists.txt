# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# A plugin and an application that lists the plugins next to it.
add_subdirectory(ExamplePlugin)
add_subdirectory(ExampleApp)
# A plugin elsewhere, which NebulaStudio opens with "Open plugin...".
add_subdirectory(ExternalPlugin)
# An application that runs a project saved by NebulaStudio.
add_subdirectory(ExampleUniverseApp)

# The application loads the plugin without linking to it: build the plugin with the application.
add_dependencies(ExampleApp ExamplePlugin)
# NebulaStudio neither: without this, building NebulaStudio alone rebuilds Core and leaves these plugins as they were
# built, against the Core of before. Their components then crash as soon as they are created.
add_dependencies(NebulaStudio ExamplePlugin ExternalPlugin)
# Nor does the application of a project, whichever of these plugins its components are from.
add_dependencies(ExampleUniverseApp ExamplePlugin ExternalPlugin)
