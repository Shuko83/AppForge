#pragma once

#include <QGraphicsItem>
#include <QString>

namespace AppForge
{
class Component;
}

// A component instantiated in the edition zone: its name above the name of its component. It can be moved and selected.
class ComponentItem : public QGraphicsItem
{
  public:
    // component, owned by the Assembly, outlives the item.
    ComponentItem(AppForge::Component& component, QString componentName);

    [[nodiscard]] AppForge::Component& component() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    AppForge::Component& m_component;
    QString m_componentName;
};
