#include "LinkItem.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QWidget>
#include <algorithm>
#include <cmath>

#include "InterfaceItem.h"

namespace
{

constexpr qreal LineWidth = 1.5;
// How far, at least, a link goes straight out of its interfaces before turning to the other one.
constexpr qreal MinimumReach = 40;

} // namespace

LinkItem::LinkItem(InterfaceItem& provided, InterfaceItem& consumed) : m_provided(provided), m_consumed(consumed)
{
    setZValue(-1); // Under the components: it only shows between them
    m_provided.addLink(*this);
    m_consumed.addLink(*this);
    follow();
}

LinkItem::~LinkItem()
{
    m_provided.removeLink(*this);
    m_consumed.removeLink(*this);
}

QPainterPath LinkItem::curve(const QPointF& provided, const QPointF& consumed)
{
    // Out of the right edge of the provider, into the left edge of the consumer, wherever they are.
    const QPointF reach(std::max(MinimumReach, std::abs(consumed.x() - provided.x()) / 2), 0);
    QPainterPath path(provided);
    path.cubicTo(provided + reach, consumed - reach, consumed);
    return path;
}

void LinkItem::follow()
{
    prepareGeometryChange();
    m_curve = curve(m_provided.scenePos(), m_consumed.scenePos());
}

QRectF LinkItem::boundingRect() const
{
    return m_curve.boundingRect().adjusted(-LineWidth, -LineWidth, LineWidth, LineWidth);
}

void LinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(QPalette::ButtonText), LineWidth));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_curve);
}
