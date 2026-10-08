#include "Plugin/Plugin.h"

// Without component: only its metadata is read.
class NewerCorePlugin : public AppForge::Plugin
{
};

APPFORGE_PLUGIN(NewerCorePlugin);
