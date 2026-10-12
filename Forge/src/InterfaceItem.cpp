#include "InterfaceItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QToolTip>
#include <QWidget>
#include <numbers>

#include "AppForge/Component/InterfaceInfo.h"
#include "Colors.h"
#include "ComponentItem.h"
#include "LinkItem.h"

namespace
{

constexpr qreal DotRadius = 4.5;
constexpr qreal DotOutline = 1.5;
// How many dots make the outline of an unavailable interface.
constexpr int OutlineDots = 6;
// Around the dot, where hovering still shows its tooltip.
constexpr qreal HoverMargin = 3;
// Between the dot and the name.
constexpr qreal Spacing = 5;
// How much of its color a dimmed interface keeps.
constexpr float DimmedOpacity = 0.3F;

// An outline of the dot made of OutlineDots dashes, as long as the gaps between them.
QPen dottedOutline(const QColor& color)
{
    const qreal step = 2 * std::numbers::pi * DotRadius / (2 * OutlineDots);
    QPen pen(color, DotOutline);
    pen.setCapStyle(Qt::FlatCap);
    pen.setDashPattern({step / DotOutline, step / DotOutline}); // In units of the width
    return pen;
}

} // namespace

InterfaceItem::InterfaceItem(Kind kind, const AppForge::InterfaceInfo& info, qreal nameWidth, ComponentItem& parent)
    : QGraphicsItem(&parent), m_componentItem(parent), m_kind(kind), m_name(info.name), m_type(info.type),
      m_nameWidth(nameWidth)
{
    setFlag(ItemSendsScenePositionChanges); // For its links to follow it
    setAcceptHoverEvents(true);
    setCursor(Qt::CrossCursor);
    setAvailable(info.available);
}

InterfaceItem::~InterfaceItem()
{
    // Each one removes itself from the list.
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst();
    }
}

int InterfaceItem::type() const
{
    return Type;
}

InterfaceItem::Kind InterfaceItem::kind() const
{
    return m_kind;
}

const QString& InterfaceItem::name() const
{
    return m_name;
}

AppForge::Component& InterfaceItem::component() const
{
    return m_componentItem.component();
}

bool InterfaceItem::canLinkTo(const InterfaceItem& other) const
{
    return m_kind != other.m_kind && m_type == other.m_type && &component() != &other.component();
}

void InterfaceItem::setAvailable(bool available)
{
    m_available = available;
    // The type is the one of the pointer: what it points to is the interface.
    QString typeName = QString::fromLatin1(m_type.name());
    if(typeName.endsWith(u'*'))
    {
        typeName.chop(1);
    }
    // What it is, which tells what it can be bound to: the other kind, of the same type.
    const QString what = (m_kind == Kind::Provided ? tr("Provides %1") : tr("Consumes %1")).arg(typeName);
    setToolTip(QStringLiteral("%1\n%2").arg(what, m_available ? tr("Available") : tr("Unavailable")));
    update();
}

void InterfaceItem::setDimmed(bool dimmed)
{
    if(m_dimmed == dimmed)
    {
        return;
    }
    m_dimmed = dimmed;
    update();
}

const QList<LinkItem*>& InterfaceItem::links() const
{
    return m_links;
}

void InterfaceItem::addLink(LinkItem& link)
{
    m_links.append(&link);
}

void InterfaceItem::removeLink(LinkItem& link)
{
    m_links.removeOne(&link);
}

QRectF InterfaceItem::boundingRect() const
{
    // From what shape() takes around its dot to its name, which is inside the component.
    const qreal dot = DotRadius + HoverMargin;
    const qreal width = dot + DotRadius + Spacing + m_nameWidth;
    return {m_kind == Kind::Provided ? dot - width : -dot, -RowHeight / 2, width, RowHeight};
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
    // The colors of the component, as ComponentItem paints them over the background of the edition zone. Opaque: the
    // colors of a style can be translucent, which would let the edge of the component show through the dot.
    const QColor background = over(palette.color(QPalette::Button), palette.color(QPalette::Base));
    // Dimmed, it fades into the component, but stays opaque over its edge.
    QColor ink = palette.color(QPalette::ButtonText);
    ink.setAlphaF(m_dimmed ? ink.alphaF() * DimmedOpacity : ink.alphaF());
    const QColor color = over(ink, background);

    // Shapes rather than colors, which not everyone tells apart: a disc is provided, a circle is consumed, and a dotted
    // circle is provided but unavailable. Not filled, it still hides the edge of the component it is on.
    const bool provided = m_kind == Kind::Provided;
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(provided && !m_available ? dottedOutline(color) : QPen(color, DotOutline));
    painter->setBrush(provided && m_available ? color : background);
    painter->drawEllipse(QPointF(0, 0), DotRadius, DotRadius);

    const qreal offset = DotRadius + Spacing;
    const QRectF nameRect(provided ? -offset - m_nameWidth : offset, -RowHeight / 2, m_nameWidth, RowHeight);
    painter->setPen(color);
    painter->drawText(nameRect, (provided ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter,
                      QFontMetricsF(painter->font()).elidedText(m_name, Qt::ElideRight, nameRect.width()));
}

void InterfaceItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    // At once, without the delay of a tooltip: its dot is small, and its type is what is looked for there.
    QToolTip::showText(event->screenPos(), toolTip(), event->widget());
}

void InterfaceItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/)
{
    QToolTip::hideText();
}

QVariant InterfaceItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if(change == ItemScenePositionHasChanged)
    {
        for(LinkItem* link : std::as_const(m_links))
        {
            link->follow();
        }
    }
    return QGraphicsItem::itemChange(change, value);
}
