#pragma once

#include "Plugin/Plugin.h"

class Label;
class Window;

// Class of the plugin: builds each of its components with the class holding its logic.
class WidgetPlugin : public AppForge::Plugin
{
  public:
    void build(Label& label);
    void build(Window& window);
};
