#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QString>

namespace AppForge
{
struct InterfaceInfo;
}

// An interface a component provides, on an edge of its ComponentItem: a dot, with its name next to it, inside the
// component. The dot is filled while the interface is available and hollow otherwise; hovering it tells its type.
class InterfaceItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(InterfaceItem)

  public:
    // What an interface takes of the height of its component.
    static constexpr qreal RowHeight = 20;

    // Its position is the center of its dot, on the right edge of parent; its name fits in nameWidth, left of the dot.
    InterfaceItem(const AppForge::InterfaceInfo& info, qreal nameWidth, QGraphicsItem* parent);

    void setAvailable(bool available);

    [[nodiscard]] QRectF boundingRect() const override;
    // Its dot only, which its tooltip is about.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    QString m_name;
    QString m_typeName;
    qreal m_nameWidth;
    bool m_available = false;
};
