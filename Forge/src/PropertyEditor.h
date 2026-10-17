// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QPointer>
#include <QString>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;

namespace Nebula
{
class Component;
class ComponentFactory;
class ForgeApplication;
} // namespace Nebula

// Edits the properties of a component instantiated in the application, then those of the objects it added with
// Component::addPropertiesForQObject(): with its own when they were added without a name, under the name they were
// added with otherwise, one group per name. It only shows the properties it can edit: they can be written and their
// type has an editor. It follows the changes the component and its objects notify, and the objects it adds or loses.
// The name of a property a settings file of the application holds is in italics: its value is the default one.
// While the application runs, it only shows the values: they are edited once it is stopped, which gives them back the
// values of the edition.
class PropertyEditor : public QWidget
{
    Q_OBJECT

  public:
    // The components are those of application.
    PropertyEditor(const Nebula::ForgeApplication& application, const Nebula::ComponentFactory& factory,
                   QWidget* parent = nullptr);

    // Shows the editable properties of component; none when it is nullptr.
    void setComponent(Nebula::Component* component);

  private slots:
    // Shows the values the properties have now. A slot: their notify signals are only known as QMetaMethod.
    void refreshValues();

  private:
    // A row of m_table: a property of object, or the title of a group, which has no property.
    struct Row
    {
        QPointer<QObject> object;
        QMetaProperty property;
        QString title;
    };

    // Shows the properties m_component and its objects have now.
    void rebuild();
    // Tells why no property is shown, when none is. It names the component, which can be renamed meanwhile.
    void refreshPlaceholder();
    // Writes the value edited in item to its property.
    void write(const QTableWidgetItem* item);

    const Nebula::ForgeApplication& m_application;
    const Nebula::ComponentFactory& m_factory;
    QPointer<Nebula::Component> m_component;
    QList<Row> m_rows; // One per row of m_table

    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr; // Shown when there is no property to show, and tells why
    QTableWidget* m_table = nullptr; // Shown otherwise
};
