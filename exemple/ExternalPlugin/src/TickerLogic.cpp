#include "TickerLogic.h"

#include "Ticker.h"

TickerLogic::TickerLogic(Ticker& ticker) : QObject(&ticker), m_ticker(ticker)
{
    connect(&m_timer, &QTimer::timeout, this, [this] { m_ticker.setTicks(m_ticker.ticks() + 1); });
    connect(&m_ticker, &Ticker::intervalChanged, this, [this](int interval) { m_timer.setInterval(interval); });
    connect(&m_ticker, &AppForge::Component::stateChanged, this, &TickerLogic::onStateChanged);
}

void TickerLogic::onStateChanged(AppForge::ComponentState state)
{
    if(state == AppForge::ComponentState::Running)
    {
        m_timer.start(m_ticker.interval());
    }
    else
    {
        m_timer.stop();
    }
}
