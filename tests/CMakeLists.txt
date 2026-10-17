# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# The tests of Nebula: a QTest executable per class tested, each one a test of ctest. See docs/testing.md.
# ENVIRONMENT_MODIFICATION, which tells them where Core and Qt are, needs CMake 3.22.
if(CMAKE_VERSION VERSION_LESS 3.22)
    message(WARNING "The tests of Nebula need CMake 3.22: they are not built")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Test Svg)

# Apart from the applications: their plugins/ would hold nothing the tests need, and what the tests build is not
# shipped. Multi-config generators add a <config> subdirectory: <build>/tests/bin/Debug with Visual Studio.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")

# The components the tests instantiate, registered in ComponentFactory::instance() by the test itself.
add_subdirectory(Components)

# Adds the test <NAME>, a QTest executable built from <NAME>.cpp, in the current directory, and SOURCES.
#   nebula_add_test(<NAME> [SOURCES <file>...] [LINK_LIBRARIES <library>...] [INCLUDE_DIRECTORIES <directory>...])
# It runs on the Core and the Qt of the build, with no window shown (QT_QPA_PLATFORM=offscreen): ctest, and the
# debugger of Visual Studio, find them through PATH. It has the directory of the example plugins, built before it, in
# NEBULA_EXAMPLE_PLUGINS_DIR and the one of ExternalPlugin in NEBULA_EXTERNAL_PLUGINS_DIR, and the version of the
# project in NEBULA_VERSION.
function(nebula_add_test NAME)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;LINK_LIBRARIES;INCLUDE_DIRECTORIES")
    add_executable(${NAME} ${NAME}.cpp ${ARG_SOURCES})
    target_link_libraries(${NAME} PRIVATE Qt6::Test ${ARG_LINK_LIBRARIES})
    target_include_directories(${NAME} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
    target_compile_definitions(${NAME} PRIVATE
        "NEBULA_EXAMPLE_PLUGINS_DIR=\"$<TARGET_FILE_DIR:ExamplePlugin>\""
        "NEBULA_EXTERNAL_PLUGINS_DIR=\"$<TARGET_FILE_DIR:ExternalPlugin>\""
        "NEBULA_VERSION=\"${PROJECT_VERSION}\"")
    set_target_properties(${NAME} PROPERTIES AUTOMOC ON AUTORCC ON FOLDER tests)
    add_dependencies(${NAME} ExamplePlugin ExternalPlugin)

    add_test(NAME ${NAME} COMMAND ${NAME})
    # Qt first: Core.dll is next to the Qt NebulaStudio was deployed with, which lacks the offscreen platform.
    # QT_FORCE_STDERR_LOGGING: on Windows, without a console, Qt writes its messages, the results of QTest included,
    # to the debugger only.
    set(_environment
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Nebula::Core>"
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        "QT_QPA_PLATFORM=set:offscreen"
        "QT_FORCE_STDERR_LOGGING=set:1")
    set_tests_properties(${NAME} PROPERTIES ENVIRONMENT_MODIFICATION "${_environment}")
    set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$<TARGET_FILE_DIR:Nebula::Core>;%PATH%\nQT_QPA_PLATFORM=offscreen")
endfunction()

add_subdirectory(Core)
add_subdirectory(NebulaStudio)
