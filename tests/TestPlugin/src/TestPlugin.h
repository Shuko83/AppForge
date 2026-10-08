#pragma once

#include "Plugin/Plugin.h"

class Probe;

// Class of the plugin: tells each Probe it builds that it built it.
class TestPlugin : public AppForge::Plugin
{
  public:
    void build(Probe& probe);
};
