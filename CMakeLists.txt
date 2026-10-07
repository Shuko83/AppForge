cmake_minimum_required(VERSION 3.16)

project(AppForge)

include(${CMAKE_CURRENT_LIST_DIR}/CMakeUtils/CMakeUtils.cmake)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Executables and DLLs side by side, so that Core.dll is found when an application starts;
# appforge_add_plugin puts the plugins in plugins/ next to them.
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

add_subdirectory(Core)
add_subdirectory(Forge)
add_subdirectory(exemple)

