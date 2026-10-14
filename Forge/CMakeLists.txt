# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The application that assembles applications from the components of the plugins, live.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Its logo, in resources/, is the icon of its windows: forge.png, through forge.qrc.
cmu_add_executable(
    NAME Forge
    NAMESPACE AppForge
    SOURCES ../resources/forge.qrc
    LINK_LIBRARIES PRIVATE AppForge::Core Qt6::Widgets
)
set_target_properties(Forge PROPERTIES WIN32_EXECUTABLE ON)
# And the icon of Forge.exe: forge.ico, through forge.rc.
if(WIN32)
    target_sources(Forge PRIVATE resources/forge.rc)
endif()
