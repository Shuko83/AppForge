// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Plugin/Plugin.h"

class Greeter;

// Class of the plugin: builds each of its components with the class holding its logic.
class ExamplePlugin : public Nebula::Plugin
{
  public:
    void build(Greeter& greeter);
};
