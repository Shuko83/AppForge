# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Runs the application of the project file given on its command line, as NebulaStudio saved it.
cmu_add_executable(
    NAME ExampleUniverseApp
    NAMESPACE Nebula
    # Widgets: the components of a project may show some, as those of ExternalPlugin do.
    LINK_LIBRARIES PRIVATE Nebula::Core Qt6::Widgets
)
