# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Components derive from QObject: moc must run on the public headers.
set(CMAKE_AUTOMOC ON)

cmu_add_shared_library(
    NAME Core
    NAMESPACE Nebula
    LINK_LIBRARIES PUBLIC Qt6::Core
)

# nebula_add_plugin(): plugins built against this Core.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/NebulaPlugin.cmake)
# And against the installed one: find_package(Nebula COMPONENTS Core) includes it, after the CMakeUtils it calls.
cmu_add_cmake_files(
    TARGET Core
    INCLUDE cmake/NebulaPlugin.cmake
    DIRECTORIES cmake/Template
    CMAKEUTILS
)
