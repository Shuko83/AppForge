// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>

#include "AppForge/Plugin/Plugin.h"

namespace AppForge
{

class Composite;
class Gateway;

// The plugin class of the components Core provides itself, as a plugin provides its own: ComponentFactory registers
// them under ComponentFactory::corePluginId() when it is created.
class CorePlugin : public Plugin
{
  public:
    // The ids ComponentFactory registers them under, as it registers those of a plugin: <plugin id>.<class>.
    [[nodiscard]] static QString compositeId();
    [[nodiscard]] static QString gatewayId();

    // A composite has no logic of its own: what fills it is its file, see ForgeApplication::instantiateComposite().
    void build(Composite& composite);
    // Nor has a gateway: its composite attaches it.
    void build(Gateway& gateway);
};

} // namespace AppForge
