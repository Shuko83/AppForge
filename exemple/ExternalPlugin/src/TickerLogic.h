#pragma once

#include <QObject>
#include <QTimer>

#include "Component/IComponent.h"

class Ticker;

// Logic of a Ticker, built with it by ExternalPlugin: counts the ticks of a timer while the Ticker runs.
class TickerLogic : public QObject
{
    Q_OBJECT

  public:
    // A child of ticker, destroyed with it.
    explicit TickerLogic(Ticker& ticker);

  private:
    void onStateChanged(AppForge::ComponentState state);

    Ticker& m_ticker;
    QTimer m_timer;
};
