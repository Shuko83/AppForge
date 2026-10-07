#include "ExamplePlugin.h"

#include "Greeter.h"
#include "GreeterLogic.h"

APPFORGE_PLUGIN(ExamplePlugin);

void ExamplePlugin::build(Greeter& greeter)
{
    // Owned by greeter, as a child QObject: it lives as long as the component.
    new GreeterLogic(greeter);
}
