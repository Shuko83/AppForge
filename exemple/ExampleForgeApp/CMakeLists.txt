# Runs the application of the project file given on its command line, as Forge saved it.
cmu_add_executable(
    NAME ExampleForgeApp
    NAMESPACE AppForge
    # Widgets: the components of a project may show some, as those of ExternalPlugin do.
    LINK_LIBRARIES PRIVATE AppForge::Core Qt6::Widgets
)
