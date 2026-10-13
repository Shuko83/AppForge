// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ExternalPlugin.h"

#include "Ticker.h"
#include "TickerLogic.h"
#include "Window.h"
#include "WindowLogic.h"

APPFORGE_PLUGIN(ExternalPlugin);

void ExternalPlugin::build(Ticker& ticker)
{
    // Owned by ticker, as a child QObject: it lives as long as the component.
    new TickerLogic(ticker);
}

void ExternalPlugin::build(Window& window)
{
    new WindowLogic(window);
}
