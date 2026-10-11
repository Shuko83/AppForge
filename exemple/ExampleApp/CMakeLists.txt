# Lists the plugins of <application directory>/plugins and loads them.
cmu_add_executable(
    NAME ExampleApp
    NAMESPACE AppForge
    LINK_LIBRARIES PRIVATE AppForge::Core
)
