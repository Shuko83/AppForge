// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Nebula/Component/Component.h"

class QTimer;

// Consumes the interface "object", a QObject it needs, and "timer", a QTimer it can do without: both declared as it is
// built, as a project needs them to bind them.
class Sink : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Consumes an object and a timer")
    Q_CLASSINFO("category", "Test")

  public:
    Sink();

    // What it has of "object"; nullptr while it is unavailable.
    [[nodiscard]] QObject* object() const;

  private:
    QObject* m_object = nullptr;
    QTimer* m_timer = nullptr;
};
