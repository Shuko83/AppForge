#pragma once

#include "Plugin/Plugin.h"

class Ticker;

// Class of the plugin: builds each of its components with the class holding its logic.
class ExternalPlugin : public AppForge::Plugin
{
  public:
    void build(Ticker& ticker);
};
