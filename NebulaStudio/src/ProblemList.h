// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTreeWidget;

namespace Nebula
{
class Universe;
}

// What keeps the components of the application from working, one row per interface one of them cannot do without and
// that is bound to nothing, see Component::unboundMandatoryInterfaces(): the component, the interface, and how to bind
// it, naming the interfaces of its type the other components provide. A sentence tells when there is none. It follows
// the components the application instantiates and removes, their names, and their interfaces and links. Activating a
// row, by double-clicking it or pressing Enter, asks for its component: see componentActivated().
class ProblemList : public QWidget
{
    Q_OBJECT

  public:
    explicit ProblemList(const Nebula::Universe& application, QWidget* parent = nullptr);

  signals:
    // The row of the component named name was activated: it is to be shown.
    void componentActivated(const QString& name);

  private slots:
    // Refreshes it once the events being handled are: what changes the components, a removal above all, tells it
    // while they are still changing.
    void scheduleRefresh();

  private:
    // Lists what keeps the components from working now, and follows each component.
    void refresh();

    const Nebula::Universe& m_application;
    bool m_refreshScheduled = false;

    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr; // Shown when every component works
    QTreeWidget* m_tree = nullptr;   // Shown otherwise
};
