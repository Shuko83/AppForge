# A plugin built against the next major version of Core: PluginManager must refuse it.
# appforge_add_plugin writes the version of Core held by APPFORGE_CORE_VERSION in its metadata.
get_property(_core_version GLOBAL PROPERTY APPFORGE_CORE_VERSION)
string(REGEX MATCH "^[0-9]+" _core_major "${_core_version}")
math(EXPR _next_major "${_core_major} + 1")
set_property(GLOBAL PROPERTY APPFORGE_CORE_VERSION "${_next_major}.0.0")

appforge_add_plugin(NAME NewerCorePlugin)

set_property(GLOBAL PROPERTY APPFORGE_CORE_VERSION "${_core_version}")
