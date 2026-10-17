// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLCDNumber>
#include <QObject>
#include <QTimer>

#include "Nebula/Component/IComponent.h"

class Ticker;

// Logic of a Ticker, built with it by ExternalPlugin: counts the ticks of a timer while the Ticker runs, and shows
// them in the display the Ticker provides.
class TickerLogic : public QObject
{
    Q_OBJECT

  public:
    // A child of ticker, destroyed with it.
    explicit TickerLogic(Ticker& ticker);

  private:
    void onStateChanged(Nebula::ComponentState state);

    Ticker& m_ticker;
    QTimer m_timer;
    QLCDNumber m_display;
};
