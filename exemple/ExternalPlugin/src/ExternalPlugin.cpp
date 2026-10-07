#include "ExternalPlugin.h"

#include "Ticker.h"
#include "TickerLogic.h"

APPFORGE_PLUGIN(ExternalPlugin);

void ExternalPlugin::build(Ticker& ticker)
{
    // Owned by ticker, as a child QObject: it lives as long as the component.
    new TickerLogic(ticker);
}
