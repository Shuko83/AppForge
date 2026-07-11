cmake_minimum_required(VERSION 3.21)

project(AppForge)

if(PROJECT_IS_TOP_LEVEL)
    include("${CMAKE_SOURCE_DIR}/CFmake/CSToolkit.cmake")
endif()

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)


add_subdirectory(Core)
