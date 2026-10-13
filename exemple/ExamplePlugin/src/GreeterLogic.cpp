// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "GreeterLogic.h"

#include "Greeter.h"

GreeterLogic::GreeterLogic(Greeter& greeter) : QObject(&greeter), m_greeter(greeter)
{
    connect(&m_greeter, &AppForge::Component::stateChanged, this, &GreeterLogic::onStateChanged);
}

QString GreeterLogic::greet(const QString& recipient) const
{
    return QStringLiteral("Hello, %1!").arg(recipient);
}

void GreeterLogic::onStateChanged(AppForge::ComponentState state)
{
    if(state == AppForge::ComponentState::Running)
    {
        m_greeter.setGreeting(greet(m_greeter.recipient()));
        m_greeter.setGreeter(this);
    }
    else
    {
        m_greeter.setGreeter(nullptr);
    }
}
