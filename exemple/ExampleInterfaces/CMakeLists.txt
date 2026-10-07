# Interfaces the example components provide and consume: shared by the plugins and the applications, they belong to
# none of them.
cmu_add_interface_library(
    NAME ExampleInterfaces
    LINK_LIBRARIES INTERFACE AppForge::Core
)
