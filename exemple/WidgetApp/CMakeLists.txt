# Binds a Window to a Label from WidgetPlugin: the Window shows the widget the Label provides.
cmu_add_executable(
    NAME WidgetApp
    LINK_LIBRARIES PRIVATE AppForge::Core ExampleInterfaces Qt6::Widgets
)
