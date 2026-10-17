# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The application that assembles applications from the components of the plugins, live.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Its icons and the logo of Nebula are SVG files, in resources/ through nebulastudio.qrc: Qt Svg draws the logo on the
# splash screen, and its icon engine, deployed with it, reads the icons.
find_package(Qt6 REQUIRED COMPONENTS Svg)
cmu_add_executable(
    NAME NebulaStudio
    NAMESPACE Nebula
    SOURCES ../resources/nebulastudio.qrc
    LINK_LIBRARIES PRIVATE Nebula::Core Qt6::Widgets Qt6::Svg
)
set_target_properties(NebulaStudio PROPERTIES WIN32_EXECUTABLE ON)
# And the icon of NebulaStudio.exe: nebula.ico, rendered from the logo, through nebulastudio.rc.
if(WIN32)
    target_sources(NebulaStudio PRIVATE resources/nebulastudio.rc)
endif()
