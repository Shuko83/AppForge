#include "TestPlugin.h"

#include "Probe.h"

APPFORGE_PLUGIN(TestPlugin);

void TestPlugin::build(Probe& probe)
{
    probe.setBuiltBy(QStringLiteral("TestPlugin"));
}
