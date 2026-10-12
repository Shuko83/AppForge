#pragma once

#include <QList>
#include <QMetaProperty>
#include <QPointer>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;

namespace AppForge
{
class Component;
class ComponentFactory;
} // namespace AppForge

// Edits the properties of a component instantiated in the application. It only shows those it can edit: they can be
// written and their type has an editor. It follows the changes the component notifies.
class PropertyEditor : public QWidget
{
    Q_OBJECT

  public:
    explicit PropertyEditor(const AppForge::ComponentFactory& factory, QWidget* parent = nullptr);

    // Shows the editable properties of component; none when it is nullptr.
    void setComponent(AppForge::Component* component);

  private slots:
    // Shows the values the properties have now. A slot: their notify signals are only known as QMetaMethod.
    void refreshValues();

  private:
    // Writes the value edited in item to its property.
    void write(const QTableWidgetItem* item);

    const AppForge::ComponentFactory& m_factory;
    QPointer<AppForge::Component> m_component;
    QList<QMetaProperty> m_properties; // Those of m_component that can be edited, one per row of m_table

    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr; // Shown when there is no property to show, and tells why
    QTableWidget* m_table = nullptr; // Shown otherwise
};
