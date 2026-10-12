#pragma once

#include <QColor>
#include <QGraphicsObject>
#include <QList>
#include <QString>

class InterfaceItem;
class NameEditor;

namespace AppForge
{
class Component;
}

// A component instantiated in the edition zone: its name above the name of its component, then its interfaces, one
// InterfaceItem per row: those it consumes on its left edge, then those it provides on its right edge. A mark in its
// corner tells its state: a square once it is initialized, a triangle while it runs. It can be moved and selected.
// Double-clicking its names edits its name in place, and it can be given a color, which its names are written over.
class ComponentItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // component, owned by the application, outlives the item.
    ComponentItem(AppForge::Component& component, QString componentName);

    [[nodiscard]] AppForge::Component& component() const;
    // Its interfaces, in the order of Component::consumedInterfaces() and Component::providedInterfaces().
    [[nodiscard]] const QList<InterfaceItem*>& consumedInterfaces() const;
    [[nodiscard]] const QList<InterfaceItem*>& providedInterfaces() const;
    // Not valid until it is given one: it has the colors of the style. As opaque as it was chosen.
    [[nodiscard]] const QColor& color() const;
    void setColor(const QColor& color);
    // Edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. It
    // must be in a scene. See nameEdited().
    void editName();

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  signals:
    // name was typed in place of its name, on one line: its component is still to be named so.
    void nameEdited(const QString& name);

  protected:
    // Not moved while its name is edited: the double-click that edits it still holds it, and the view scrolling to
    // show what is typed would drag it along.
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

  private:
    // Whether its name is being edited in place.
    [[nodiscard]] bool isNameEdited() const;
    // Shows the interfaces of the component as they are now: it only ever declares more of them.
    void refreshInterfaces();

    AppForge::Component& m_component;
    QString m_componentName;
    QColor m_color;
    // Its children.
    QList<InterfaceItem*> m_consumedInterfaces;
    QList<InterfaceItem*> m_providedInterfaces;
    NameEditor* m_nameEditor = nullptr; // Over its name while it is edited; created when first so
};
