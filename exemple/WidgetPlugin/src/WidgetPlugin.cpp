#include "WidgetPlugin.h"

#include "Label.h"
#include "LabelLogic.h"
#include "Window.h"
#include "WindowLogic.h"

APPFORGE_PLUGIN(WidgetPlugin);

void WidgetPlugin::build(Label& label)
{
    // Owned by label, as a child QObject: the IWidget label provides.
    label.setWidget(new LabelLogic(label));
}

void WidgetPlugin::build(Window& window)
{
    // Owned by window, as a child QObject: it lives as long as the component.
    new WindowLogic(window);
}
