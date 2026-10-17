# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Components written for the tests, which control what they provide, consume and hold. Not a plugin: a test registers
# them in its own process with registerTestComponents(), and drives them through their own classes.
file(GLOB_RECURSE _sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_library(TestComponents STATIC ${_sources})
target_include_directories(TestComponents PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(TestComponents PUBLIC Nebula::Core)
set_target_properties(TestComponents PROPERTIES AUTOMOC ON FOLDER tests)
