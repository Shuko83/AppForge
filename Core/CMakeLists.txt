# Components derive from QObject: moc must run on the public headers.
set(CMAKE_AUTOMOC ON)

cmu_add_shared_library(
    NAME Core
    NAMESPACE AppForge
    LINK_LIBRARIES PUBLIC Qt6::Core
)

# appforge_add_plugin(): plugins built against this Core.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/AppForgePlugin.cmake)
