include_guard(GLOBAL)

# appforge_add_plugin runs in its caller's directory: what it needs from here is kept in global properties.
set_property(GLOBAL PROPERTY APPFORGE_PLUGIN_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
# Version of Core the plugins are built against: same rule as cmu_add_target for CoreInfo::version.
if(PROJECT_VERSION)
    set_property(GLOBAL PROPERTY APPFORGE_CORE_VERSION "${PROJECT_VERSION}")
else()
    set_property(GLOBAL PROPERTY APPFORGE_CORE_VERSION "0.0.0")
endif()

# Creates a plugin loaded by AppForge::PluginManager: a MODULE library carrying its metadata (see README.md).
#   appforge_add_plugin(NAME <name> [DESCRIPTION <text>] [VERSION <version>] [<cmu_add_target keywords>...])
function(appforge_add_plugin)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME;DESCRIPTION;VERSION" "")
    if(NOT DEFINED ARG_NAME)
        message(FATAL_ERROR "appforge_add_plugin: NAME is required")
    endif()

    # <organization>.<project>.<target>: a dot or a space in a part would make the id ambiguous.
    foreach(_part IN ITEMS "${cmu_organization}" "${PROJECT_NAME}" "${ARG_NAME}")
        if(NOT _part MATCHES "^[A-Za-z0-9_-]+$")
            message(FATAL_ERROR "appforge_add_plugin(${ARG_NAME}): '${_part}' cannot be part of the plugin id "
                "(letters, digits, '_' and '-' only)")
        endif()
    endforeach()
    set(_id "${cmu_organization}.${PROJECT_NAME}.${ARG_NAME}")

    set(_forwarded ${ARG_UNPARSED_ARGUMENTS})
    if(DEFINED ARG_VERSION)
        list(APPEND _forwarded VERSION "${ARG_VERSION}")
    endif()
    cmu_add_target(NAME ${ARG_NAME} ${_forwarded} TYPE MODULE)
    target_link_libraries(${ARG_NAME} PRIVATE AppForge::Core)
    target_compile_definitions(${ARG_NAME} PRIVATE "APPFORGE_PLUGIN_ID=\"${_id}\"")

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
        SUFFIX ".afplugin"
        LIBRARY_OUTPUT_DIRECTORY "${_output_dir}/plugins")

    # Values of Template/plugin.json.in, a JSON string for the description.
    set(APPFORGE_PLUGIN_ID "${_id}")
    set(APPFORGE_PLUGIN_NAME "${ARG_NAME}")
    string(REPLACE "\\" "\\\\" APPFORGE_PLUGIN_DESCRIPTION "${ARG_DESCRIPTION}")
    string(REPLACE "\"" "\\\"" APPFORGE_PLUGIN_DESCRIPTION "${APPFORGE_PLUGIN_DESCRIPTION}")
    string(REPLACE "\n" "\\n" APPFORGE_PLUGIN_DESCRIPTION "${APPFORGE_PLUGIN_DESCRIPTION}")
    string(REPLACE "\t" "\\t" APPFORGE_PLUGIN_DESCRIPTION "${APPFORGE_PLUGIN_DESCRIPTION}")
    if(DEFINED ARG_VERSION)
        set(APPFORGE_PLUGIN_VERSION "${ARG_VERSION}")
    elseif(PROJECT_VERSION)
        set(APPFORGE_PLUGIN_VERSION "${PROJECT_VERSION}")
    else()
        set(APPFORGE_PLUGIN_VERSION "0.0.0")
    endif()
    get_property(APPFORGE_PLUGIN_CORE_VERSION GLOBAL PROPERTY APPFORGE_CORE_VERSION)
    # Kept as is: AppForgePluginMetadata.cmake replaces it at build time.
    set(APPFORGE_PLUGIN_BUILD_DATE "@APPFORGE_PLUGIN_BUILD_DATE@")
    string(MAKE_C_IDENTIFIER "${ARG_NAME}" _identifier)
    set(APPFORGE_PLUGIN_CLASS "${_identifier}Plugin")

    # Outside <target>_autogen, which the clean target deletes: these files are only written at configure time.
    get_property(_cmake_dir GLOBAL PROPERTY APPFORGE_PLUGIN_CMAKE_DIR)
    set(_dir "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}_appforge")
    set(_header "${_dir}/${ARG_NAME}_plugin.h")
    set(_metadata "${_dir}/${ARG_NAME}_plugin.json")
    configure_file("${_cmake_dir}/Template/plugin.h.in" "${_header}" @ONLY)
    configure_file("${_cmake_dir}/Template/plugin.json.in" "${_metadata}.in" @ONLY)
    target_sources(${ARG_NAME} PRIVATE "${_header}")
    source_group("autogen\\appforge" FILES "${_header}")

    # The metadata is dated when a source of the plugin changes; AUTOMOC tracks the file of Q_PLUGIN_METADATA,
    # so moc embeds the new date and the plugin is relinked, and nothing is rebuilt otherwise.
    get_target_property(_sources ${ARG_NAME} SOURCES)
    # cmu_add_target only adds the sources of cmu_sources_dir to the target: watch its private headers as well.
    if(DEFINED cmu_sources_dir AND DEFINED cmu_headers_extension)
        get_filename_component(_sources_dir "${cmu_sources_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        foreach(_extension IN LISTS cmu_headers_extension)
            file(GLOB_RECURSE _headers CONFIGURE_DEPENDS "${_sources_dir}/*.${_extension}")
            list(APPEND _sources ${_headers})
        endforeach()
    endif()
    set(_script "${_cmake_dir}/AppForgePluginMetadata.cmake")
    add_custom_command(
        OUTPUT "${_metadata}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${_metadata}.in" "-DOUTPUT=${_metadata}" -P "${_script}"
        DEPENDS "${_metadata}.in" "${_script}" ${_sources}
        COMMENT "AppForge: dating the metadata of ${ARG_NAME}"
        VERBATIM)
    # moc reads the metadata: it must be written first.
    set_property(TARGET ${ARG_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS "${_metadata}")
endfunction()
