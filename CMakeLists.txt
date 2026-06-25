cmake_minimum_required(VERSION 3.21)

project(AppForge)

if(PROJECT_IS_TOP_LEVEL)
    include("${CMAKE_SOURCE_DIR}/CFmake/CFMake.cmake")
endif()

add_subdirectory(AfCore)