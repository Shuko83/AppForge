# Unit tests of Core, written with Qt Test and run by ctest.
# Qt6Test rather than Qt6 COMPONENTS Test: CMakeUtils repeats the find_package(Qt6 ...) calls in the Config of the
# installed targets linked to Qt6, which do not need Qt Test.
find_package(Qt6Test REQUIRED)

set(CMAKE_AUTOMOC ON)

# Plugins PluginManagerTest reads and loads, outside <application directory>/plugins so that Forge and the examples do
# not list them. Multi-config generators add a <config> subdirectory: <build>/tests/plugins/Debug with Visual Studio.
add_subdirectory(TestPlugin)
add_subdirectory(NewerCorePlugin)
set_target_properties(TestPlugin NewerCorePlugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/plugins")
# cmu_add_target puts every target in the package of the project: not these ones.
get_property(_package_targets GLOBAL PROPERTY CMU_PACKAGE_TARGETS)
list(REMOVE_ITEM _package_targets TestPlugin NewerCorePlugin)
set_property(GLOBAL PROPERTY CMU_PACKAGE_TARGETS ${_package_targets})

# Adds the test <name>, built from <name>.cpp. A plain executable rather than cmu_add_executable: neither installed
# nor packaged.
function(appforge_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE AppForge::Core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    # Core.dll is next to the test (CMAKE_RUNTIME_OUTPUT_DIRECTORY), the DLLs of Qt in its own directory.
    # Without a console, Qt Test writes its results to the Windows debugger output: ctest would show nothing.
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_FORCE_STDERR_LOGGING=1"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endfunction()

appforge_add_test(ComponentTest)
appforge_add_test(ComponentFactoryTest)
appforge_add_test(PluginManagerTest)

get_property(_core_version GLOBAL PROPERTY APPFORGE_CORE_VERSION)
target_compile_definitions(PluginManagerTest PRIVATE
    "TEST_PLUGIN_ID=\"${cmu_organization}.${PROJECT_NAME}.TestPlugin\""
    "TEST_PLUGIN_FILE=\"$<TARGET_FILE:TestPlugin>\""
    "NEWER_CORE_PLUGIN_FILE=\"$<TARGET_FILE:NewerCorePlugin>\""
    "CORE_VERSION=\"${_core_version}\"")
# PluginManagerTest loads the plugins without linking to them: build them with it.
add_dependencies(PluginManagerTest TestPlugin NewerCorePlugin)
