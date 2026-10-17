# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The classes of NebulaStudio that do not need its window, compiled again with each test: NebulaStudio is an
# executable, nothing links to it.
set(_studio "${PROJECT_SOURCE_DIR}/NebulaStudio")
# The icons and the font of the theme.
set(_resources "${_studio}/resources/nebulastudio.qrc")

nebula_add_test(TestColors INCLUDE_DIRECTORIES "${_studio}/src" LINK_LIBRARIES Qt6::Gui)
nebula_add_test(TestTheme
    SOURCES "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Qt6::Widgets Qt6::Svg)
nebula_add_test(TestPluginModel
    SOURCES "${_studio}/src/PluginModel.cpp"
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES Nebula::Core Qt6::Gui)
nebula_add_test(TestComponentModel
    SOURCES "${_studio}/src/ComponentModel.cpp" "${_studio}/src/Theme.cpp" ${_resources}
    INCLUDE_DIRECTORIES "${_studio}/src"
    LINK_LIBRARIES TestComponents Qt6::Widgets Qt6::Svg)
