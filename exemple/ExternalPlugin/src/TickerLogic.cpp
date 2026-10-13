#include "TickerLogic.h"

#include "Ticker.h"

TickerLogic::TickerLogic(Ticker& ticker) : QObject(&ticker), m_ticker(ticker)
{
    connect(&m_timer, &QTimer::timeout, this, [this] { m_ticker.setTicks(m_ticker.ticks() + 1); });
    connect(&m_ticker, &Ticker::intervalChanged, this, [this](int interval) { m_timer.setInterval(interval); });
    connect(&m_ticker, &AppForge::Component::stateChanged, this, &TickerLogic::onStateChanged);

    connect(&m_ticker, &Ticker::ticksChanged, &m_display, qOverload<int>(&QLCDNumber::display));
    m_ticker.setDisplay(&m_display);
    // Edited with the Ticker where it is edited, in a group of its own.
    m_ticker.addPropertiesForQObject(&m_display, QStringLiteral("Display"));
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
