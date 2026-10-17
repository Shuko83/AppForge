// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QString>

#include "Nebula/Component/Component.h"

class Options;

// A component with objects whose properties it adds to its own: one with them, general(), and two in the group "page",
// pages(), which a project tells apart by their order.
class Panel : public Nebula::Component
{
    Q_OBJECT
    Q_PROPERTY(QString title MEMBER m_title NOTIFY titleChanged)

  public:
    Panel();

    [[nodiscard]] Options& general() const;
    [[nodiscard]] QList<Options*> pages() const;

  signals:
    void titleChanged();

  private:
    QString m_title;
    Options* m_general; // Children of the panel
    QList<Options*> m_pages;
};
