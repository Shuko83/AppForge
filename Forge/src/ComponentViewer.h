// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QString>
#include <QWidget>

class QFormLayout;
class QLabel;
class QStackedWidget;
class QTreeWidget;

namespace AppForge
{
class ComponentFactory;
}

// Shows a component registered in a ComponentFactory: what it is and its properties. Or a composite file: its plugins
// and its components.
class ComponentViewer : public QWidget
{
    Q_OBJECT

  public:
    explicit ComponentViewer(const AppForge::ComponentFactory& factory, QWidget* parent = nullptr);

    // Shows componentId; nothing when it is empty or unknown.
    void setComponentId(const QString& componentId);
    // Shows the composite file filePath, which holds document, see ForgeApplication::saveComposite().
    void setComposite(const QString& filePath, const QJsonObject& document);

  private:
    // Lists the properties of a component, or the components of a composite.
    void showProperties(bool properties);

    const AppForge::ComponentFactory& m_factory;

    QStackedWidget* m_stack = nullptr;
    QWidget* m_placeholder = nullptr; // Shown when there is no component
    QWidget* m_page = nullptr;        // Shown otherwise
    QFormLayout* m_form = nullptr;    // Of m_page
    QLabel* m_name = nullptr;
    QLabel* m_id = nullptr;
    QLabel* m_plugin = nullptr;
    QLabel* m_category = nullptr;
    QLabel* m_description = nullptr;
    QTreeWidget* m_properties = nullptr;
};
