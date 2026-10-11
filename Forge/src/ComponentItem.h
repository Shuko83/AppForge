#pragma once

#include <QGraphicsObject>
#include <QList>
#include <QString>

class InterfaceItem;

namespace AppForge
{
class Component;
}

// A component instantiated in the edition zone: its name above the name of its component, then its interfaces, one
// InterfaceItem per row: those it consumes on its left edge, then those it provides on its right edge. It can be moved
// and selected.
class ComponentItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // component, owned by the Assembly, outlives the item.
    ComponentItem(AppForge::Component& component, QString componentName);

    [[nodiscard]] AppForge::Component& component() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    // Shows the interfaces of the component as they are now: it only ever declares more of them.
    void refreshInterfaces();

    AppForge::Component& m_component;
    QString m_componentName;
    // Its children, in the order of Component::consumedInterfaces() and Component::providedInterfaces().
    QList<InterfaceItem*> m_consumedInterfaces;
    QList<InterfaceItem*> m_providedInterfaces;
};
