// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Probe.h"

#include <QTimer>

Probe::Probe() = default;

bool Probe::provideObject(const QString& name, QObject* object)
{
    return provideInterface<QObject>(name, object);
}

bool Probe::provideTimer(const QString& name, QTimer* timer)
{
    return provideInterface<QTimer>(name, timer);
}

bool Probe::consumeObject(const QString& name, Nebula::InterfaceRequirement requirement)
{
    return consume(name, m_objects[name], requirement);
}

bool Probe::consumeTimer(const QString& name, Nebula::InterfaceRequirement requirement)
{
    return consume(name, m_timers[name], requirement);
}

QObject* Probe::consumed(const QString& name) const
{
    if(const auto object = m_objects.find(name); object != m_objects.end())
    {
        return object->second;
    }
    const auto timer = m_timers.find(name);
    return timer != m_timers.end() ? timer->second : nullptr;
}

void Probe::addObject(QObject* object, const QString& name)
{
    addPropertiesForQObject(object, name);
}

int Probe::starts() const
{
    return m_starts;
}

QStringList Probe::events() const
{
    return m_events;
}

void Probe::clearEvents()
{
    m_events.clear();
}

bool Probe::onStart()
{
    m_events.append(QStringLiteral("start"));
    if(m_refusesToStart)
    {
        return false;
    }
    ++m_starts;
    emit startsChanged();
    return true;
}

void Probe::onStop()
{
    m_events.append(QStringLiteral("stop"));
}

template <typename T> bool Probe::consume(const QString& name, T*& member, Nebula::InterfaceRequirement requirement)
{
    return consumeInterface<T>(
        name, member, requirement, [this, name] { m_events.append(QStringLiteral("available ") + name); },
        [this, name, &member]
        {
            m_events.append(QStringLiteral("unavailable ") + name +
                            (member != nullptr ? QStringLiteral(" (set)") : QString()));
        });
}
