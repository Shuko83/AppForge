// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "AppForge/Plugin/Plugin.h"

class Ticker;
class Window;

// Class of the plugin: builds each of its components with the class holding its logic.
class ExternalPlugin : public AppForge::Plugin
{
  public:
    void build(Ticker& ticker);
    void build(Window& window);
};
