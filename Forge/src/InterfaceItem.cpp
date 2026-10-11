#include "InterfaceItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QWidget>

#include "Component/InterfaceInfo.h"

namespace
{

constexpr qreal DotRadius = 4.5;
constexpr qreal DotOutline = 1.5;
// Around the dot, where hovering still shows its tooltip.
constexpr qreal HoverMargin = 3;
// Between the dot and the name.
constexpr qreal Spacing = 5;

// color as it looks over background, opaque: the colors of a style can be translucent, which would let the edge of the
// component show through the dot.
QColor over(const QColor& color, const QColor& background)
{
    const float alpha = color.alphaF();
    return QColor::fromRgbF(color.redF() * alpha + background.redF() * (1 - alpha),
                            color.greenF() * alpha + background.greenF() * (1 - alpha),
                            color.blueF() * alpha + background.blueF() * (1 - alpha));
}

} // namespace

InterfaceItem::InterfaceItem(const AppForge::InterfaceInfo& info, qreal nameWidth, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_name(info.name), m_typeName(QString::fromLatin1(info.type.name())),
      m_nameWidth(nameWidth)
{
    // The type is the one of the pointer: what it points to is the interface.
    if(m_typeName.endsWith(u'*'))
    {
        m_typeName.chop(1);
    }
    setAvailable(info.available);
}

void InterfaceItem::setAvailable(bool available)
{
    m_available = available;
    setToolTip(QStringLiteral("%1\n%2").arg(m_typeName, m_available ? tr("Available") : tr("Unavailable")));
    update();
}

QRectF InterfaceItem::boundingRect() const
{
    // From its name to what shape() takes around its dot.
    const qreal left = -DotRadius - Spacing - m_nameWidth;
    return {left, -RowHeight / 2, -left + DotRadius + HoverMargin, RowHeight};
}

QPainterPath InterfaceItem::shape() const
{
    QPainterPath path;
    path.addEllipse(QPointF(0, 0), DotRadius + HoverMargin, DotRadius + HoverMargin);
    return path;
}

void InterfaceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    // The colors of the component, as ComponentItem paints them over the background of the edition zone.
    const QColor background = over(palette.color(QPalette::Button), palette.color(QPalette::Base));
    const QColor color = over(palette.color(QPalette::ButtonText), background);

    // Filled or hollow rather than two colors: the state does not rely on telling colors apart. Hollow, it hides the
    // edge of the component it is on.
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(color, DotOutline));
    painter->setBrush(m_available ? color : background);
    painter->drawEllipse(QPointF(0, 0), DotRadius, DotRadius);

    const QRectF nameRect(-DotRadius - Spacing - m_nameWidth, -RowHeight / 2, m_nameWidth, RowHeight);
    painter->drawText(nameRect, Qt::AlignRight | Qt::AlignVCenter,
                      QFontMetricsF(painter->font()).elidedText(m_name, Qt::ElideRight, nameRect.width()));
}
