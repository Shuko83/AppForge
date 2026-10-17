# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.19)

project(Nebula VERSION 0.3.0)

# Before CMakeUtils is included, which writes them in the info files of each target. The copyright is the one of the
# license, not the one CMakeUtils would build from the organization.
set(cmu_organization "FabienDG")
set(cmu_copyright "Copyright (C) 2026 Fabien Di Grande")

include(${CMAKE_CURRENT_LIST_DIR}/CMakeUtils/CMakeUtils.cmake)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Executables and DLLs side by side, so that Core.dll is found when an application starts;
# nebula_add_plugin puts the plugins in plugins/ next to them.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

add_subdirectory(Core)
add_subdirectory(NebulaStudio)
add_subdirectory(exemple)

