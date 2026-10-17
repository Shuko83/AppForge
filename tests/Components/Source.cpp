// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Source.h"

namespace
{

const QString ObjectInterface = QStringLiteral("object");

} // namespace

Source::Source()
{
    provideInterface<QObject>(ObjectInterface);
}

QObject& Source::object()
{
    return m_object;
}

bool Source::onStart()
{
    provideInterface<QObject>(ObjectInterface, &m_object);
    return true;
}

void Source::onStop()
{
    provideInterface<QObject>(ObjectInterface, nullptr);
}
