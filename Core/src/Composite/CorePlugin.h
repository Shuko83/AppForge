// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "AppForge/Plugin/Plugin.h"

namespace AppForge
{

class Composite;

// The plugin class of the components Core provides itself, as a plugin provides its own: ComponentFactory registers
// them under ComponentFactory::corePluginId() when it is created.
class CorePlugin : public Plugin
{
  public:
    // A composite has no logic of its own: what fills it is its file, see ForgeApplication::instantiateComposite().
    void build(Composite& composite);
};

} // namespace AppForge
