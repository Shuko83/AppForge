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
class ComponentFactory;
}

// Shows a component registered in a ComponentFactory: what it is and its properties.
class ComponentViewer : public QWidget
{
    Q_OBJECT

  public:
    explicit ComponentViewer(const Nebula::ComponentFactory& factory, QWidget* parent = nullptr);

    // Shows componentId; nothing when it is empty or unknown.
    void setComponentId(const QString& componentId);

  private:
    const Nebula::ComponentFactory& m_factory;

    QStackedWidget* m_stack = nullptr;
    QWidget* m_placeholder = nullptr; // Shown when there is no component
    QWidget* m_page = nullptr;        // Shown otherwise
    QLabel* m_name = nullptr;
    QLabel* m_id = nullptr;
    QLabel* m_plugin = nullptr;
    QLabel* m_category = nullptr;
    QLabel* m_description = nullptr;
    QTreeWidget* m_properties = nullptr;
};
