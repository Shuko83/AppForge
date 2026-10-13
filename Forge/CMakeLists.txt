# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The application that assembles applications from the components of the plugins, live.
set(CMAKE_AUTOMOC ON)

cmu_add_executable(
    NAME Forge
    NAMESPACE AppForge
    LINK_LIBRARIES PRIVATE AppForge::Core Qt6::Widgets
)
set_target_properties(Forge PROPERTIES WIN32_EXECUTABLE ON)
