// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Sink.h"

#include <QTimer>

Sink::Sink()
{
    consumeInterface<QObject>(QStringLiteral("object"), m_object, Nebula::InterfaceRequirement::Mandatory);
    consumeInterface<QTimer>(QStringLiteral("timer"), m_timer);
}

QObject* Sink::object() const
{
    return m_object;
}
