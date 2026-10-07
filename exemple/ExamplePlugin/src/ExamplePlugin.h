#pragma once

#include "Plugin/Plugin.h"

class Greeter;

// Class of the plugin: builds each of its components with the class holding its logic.
class ExamplePlugin : public AppForge::Plugin
{
  public:
    void build(Greeter& greeter);
};
