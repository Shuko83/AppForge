// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Ticker.h"

#include <QWidget>

#include "ExternalPlugin.h"

NEBULA_REGISTER_COMPONENT(ExternalPlugin, Ticker);

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

void Ticker::setDisplay(QWidget* display)
{
    provideInterface<QWidget>(QStringLiteral("display"), display);
    if(display != nullptr)
    {
        // Edited and saved with its own properties, in a group of their own.
        addPropertiesForQObject(display, QStringLiteral("Display"));
    }
}
