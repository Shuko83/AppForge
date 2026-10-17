// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QObject>
#include <QString>

#include "Nebula/Component/Component.h"

// Provides the interface "object", a QObject, while it runs: declared as it is built, as a project needs it to bind it.
class Source : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Provides an object while it runs")
    Q_PROPERTY(QString label MEMBER m_label NOTIFY labelChanged)

  public:
    Source();

    // What it provides while it runs.
    [[nodiscard]] QObject& object();

  signals:
    void labelChanged();

  protected:
    bool onStart() override;
    void onStop() override;

  private:
    QString m_label;
    QObject m_object;
};
