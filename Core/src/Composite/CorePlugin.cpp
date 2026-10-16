// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "CorePlugin.h"

#include "AppForge/Component/ComponentFactory.h"

namespace AppForge
{

QString CorePlugin::compositeId()
{
    return ComponentFactory::corePluginId() + QStringLiteral(".Composite");
}

QString CorePlugin::gatewayId()
{
    return ComponentFactory::corePluginId() + QStringLiteral(".Gateway");
}

void CorePlugin::build(Composite& /*composite*/) {}

void CorePlugin::build(Gateway& /*gateway*/) {}

} // namespace AppForge
