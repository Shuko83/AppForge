# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The application that assembles applications from the components of the plugins, live.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Its logo, in resources/, is the icon of its windows: nebulastudio.png, through nebulastudio.qrc.
cmu_add_executable(
    NAME NebulaStudio
    NAMESPACE Nebula
    SOURCES ../resources/nebulastudio.qrc
    LINK_LIBRARIES PRIVATE Nebula::Core Qt6::Widgets
)
set_target_properties(NebulaStudio PROPERTIES WIN32_EXECUTABLE ON)
# And the icon of NebulaStudio.exe: nebulastudio.ico, through nebulastudio.rc.
if(WIN32)
    target_sources(NebulaStudio PRIVATE resources/nebulastudio.rc)
endif()
