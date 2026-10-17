// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "TickerLogic.h"

#include "Ticker.h"

TickerLogic::TickerLogic(Ticker& ticker) : QObject(&ticker), m_ticker(ticker)
{
    connect(&m_timer, &QTimer::timeout, this, [this] { m_ticker.setTicks(m_ticker.ticks() + 1); });
    connect(&m_ticker, &Ticker::intervalChanged, this, [this](int interval) { m_timer.setInterval(interval); });
    connect(&m_ticker, &Nebula::Component::stateChanged, this, &TickerLogic::onStateChanged);

    connect(&m_ticker, &Ticker::ticksChanged, &m_display, qOverload<int>(&QLCDNumber::display));
    m_ticker.setDisplay(&m_display);
}

void TickerLogic::onStateChanged(Nebula::ComponentState state)
{
    if(state == Nebula::ComponentState::Running)
    {
        m_timer.start(m_ticker.interval());
    }
    else
    {
        m_timer.stop();
    }
}
