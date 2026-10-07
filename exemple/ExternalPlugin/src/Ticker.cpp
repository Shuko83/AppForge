#include "Ticker.h"

#include "ExternalPlugin.h"

APPFORGE_REGISTER_COMPONENT(ExternalPlugin, Ticker);

int Ticker::interval() const
{
    return m_interval;
}

void Ticker::setInterval(int interval)
{
    if(m_interval == interval)
    {
        return;
    }
    m_interval = interval;
    emit intervalChanged(m_interval);
}

int Ticker::ticks() const
{
    return m_ticks;
}

void Ticker::setTicks(int ticks)
{
    if(m_ticks == ticks)
    {
        return;
    }
    m_ticks = ticks;
    emit ticksChanged(m_ticks);
}
