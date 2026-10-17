# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

include_guard(GLOBAL)

# nebula_add_plugin runs in its caller's directory: what it needs from here is kept in global properties.
set_property(GLOBAL PROPERTY NEBULA_PLUGIN_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
# Version of Core the plugins are built against. Installed, this file is included by find_package(Nebula), which
# sets the version of the package: PROJECT_VERSION is then the one of the project that builds the plugins.
if(CMAKE_FIND_PACKAGE_NAME STREQUAL "Nebula" AND Nebula_VERSION)
    set_property(GLOBAL PROPERTY NEBULA_CORE_VERSION "${Nebula_VERSION}")
# In the source tree: same rule as cmu_add_target for CoreInfo::version.
elseif(PROJECT_VERSION)
    set_property(GLOBAL PROPERTY NEBULA_CORE_VERSION "${PROJECT_VERSION}")
else()
    set_property(GLOBAL PROPERTY NEBULA_CORE_VERSION "0.0.0")
endif()

# Creates a plugin loaded by Nebula::PluginManager: a MODULE library carrying its metadata (see README.md).
#   nebula_add_plugin(NAME <name> [DESCRIPTION <text>] [VERSION <version>] [<cmu_add_target keywords>...])
function(nebula_add_plugin)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME;DESCRIPTION;VERSION" "")
    if(NOT DEFINED ARG_NAME)
        message(FATAL_ERROR "nebula_add_plugin: NAME is required")
    endif()

    # <organization>.<project>.<target>: a dot or a space in a part would make the id ambiguous.
    foreach(_part IN ITEMS "${cmu_organization}" "${PROJECT_NAME}" "${ARG_NAME}")
        if(NOT _part MATCHES "^[A-Za-z0-9_-]+$")
            message(FATAL_ERROR "nebula_add_plugin(${ARG_NAME}): '${_part}' cannot be part of the plugin id "
                "(letters, digits, '_' and '-' only)")
        endif()
    endforeach()
    set(_id "${cmu_organization}.${PROJECT_NAME}.${ARG_NAME}")

    set(_forwarded ${ARG_UNPARSED_ARGUMENTS})
    if(DEFINED ARG_VERSION)
        list(APPEND _forwarded VERSION "${ARG_VERSION}")
    endif()
    # .nbdl (Nebula DLL): PluginManager only scans this extension (see PluginManager.cpp). SHARED_EXTENSION rather
    # than SUFFIX: cmu_add_target also writes the file name in the version resource.
    cmu_add_target(NAME ${ARG_NAME} ${_forwarded} TYPE MODULE SHARED_EXTENSION nbdl NAMESPACE Nebula)
    target_link_libraries(${ARG_NAME} PRIVATE Nebula::Core)
    target_compile_definitions(${ARG_NAME} PRIVATE "NEBULA_PLUGIN_ID=\"${_id}\"")

    # PluginManager only scans this extension, in <application directory>/plugins (see PluginManager.cpp).
    if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else()
        set(_output_dir "${CMAKE_BINARY_DIR}/bin")
    endif()
    # Multi-config generators put the executables in a <config> subdirectory, which a generator expression disables.
    get_property(_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_multi_config)
        string(APPEND _output_dir "/$<CONFIG>")
    endif()
    set_target_properties(${ARG_NAME} PROPERTIES
        AUTOMOC ON
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${_output_dir}/plugins")

    # Values of the templates, a JSON string for the description.
    set(NEBULA_PLUGIN_ID "${_id}")
    set(NEBULA_PLUGIN_NAME "${ARG_NAME}")
    # Same identifier as the <identifier>Info namespace of the <NAME>_info.h written by cmu_add_target.
    string(MAKE_C_IDENTIFIER "${ARG_NAME}" NEBULA_PLUGIN_IDENTIFIER)
    string(REPLACE "\\" "\\\\" NEBULA_PLUGIN_DESCRIPTION "${ARG_DESCRIPTION}")
    string(REPLACE "\"" "\\\"" NEBULA_PLUGIN_DESCRIPTION "${NEBULA_PLUGIN_DESCRIPTION}")
    string(REPLACE "\n" "\\n" NEBULA_PLUGIN_DESCRIPTION "${NEBULA_PLUGIN_DESCRIPTION}")
    string(REPLACE "\t" "\\t" NEBULA_PLUGIN_DESCRIPTION "${NEBULA_PLUGIN_DESCRIPTION}")
    if(DEFINED ARG_VERSION)
        set(NEBULA_PLUGIN_VERSION "${ARG_VERSION}")
    elseif(PROJECT_VERSION)
        set(NEBULA_PLUGIN_VERSION "${PROJECT_VERSION}")
    else()
        set(NEBULA_PLUGIN_VERSION "0.0.0")
    endif()
    get_property(NEBULA_PLUGIN_CORE_VERSION GLOBAL PROPERTY NEBULA_CORE_VERSION)

    # Outside <target>_autogen, which the clean target deletes: these files are only written at configure time.
    # moc embeds the JSON (Q_PLUGIN_METADATA) and AUTOMOC runs it again when the file changes.
    get_property(_cmake_dir GLOBAL PROPERTY NEBULA_PLUGIN_CMAKE_DIR)
    set(_dir "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}_nebula")
    configure_file("${_cmake_dir}/Template/plugin.h.in" "${_dir}/${ARG_NAME}_plugin.h" @ONLY)
    configure_file("${_cmake_dir}/Template/plugin.json.in" "${_dir}/${ARG_NAME}_plugin.json" @ONLY)
    target_sources(${ARG_NAME} PRIVATE "${_dir}/${ARG_NAME}_plugin.h")
    source_group("autogen\\nebula" FILES "${_dir}/${ARG_NAME}_plugin.h")
endfunction()
