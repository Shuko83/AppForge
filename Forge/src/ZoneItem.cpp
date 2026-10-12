#include "ZoneItem.h"

#include <QCursor>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <utility>

#include "Colors.h"
#include "ComponentItem.h"
#include "NameEditor.h"

namespace
{

constexpr qreal TitleHeight = 28;
constexpr qreal Padding = 8;
constexpr qreal CornerRadius = 6;
// How far inside its bounds a zone is resized by an edge, and how far from a corner by both edges of it.
constexpr qreal EdgeWidth = 6;
constexpr qreal CornerSize = 16;
// Enough for a title, and for its corners not to meet.
constexpr QSizeF MinimumSize(120, 60);
// How much of its color a zone keeps under its title, where the components are.
constexpr float FillOpacity = 0.15F;

// The cursor of the edges a zone is resized by, at least one.
Qt::CursorShape resizeCursor(Qt::Edges edges)
{
    const bool horizontal = edges.testAnyFlags(Qt::LeftEdge | Qt::RightEdge);
    const bool vertical = edges.testAnyFlags(Qt::TopEdge | Qt::BottomEdge);
    if(horizontal && vertical)
    {
        // A corner: the diagonal it is on.
        return edges.testFlag(Qt::LeftEdge) == edges.testFlag(Qt::TopEdge) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
    }
    return horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor;
}

} // namespace

ZoneItem::ZoneItem(const QSizeF& size, QString title, const QColor& color)
    : m_size(size.expandedTo(MinimumSize)), m_title(std::move(title)), m_color(color)
{
    setAcceptHoverEvents(true);
    stack();
}

int ZoneItem::type() const
{
    return Type;
}

const QString& ZoneItem::title() const
{
    return m_title;
}

const QColor& ZoneItem::color() const
{
    return m_color;
}

void ZoneItem::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void ZoneItem::editTitle()
{
    if(m_titleEditor == nullptr)
    {
        m_titleEditor = new NameEditor(*this, [this](const QString& title) { m_title = title; });
    }
    m_titleEditor->edit(m_title, textColorOver(m_color), titleRect(), Qt::AlignLeft);
}

QRectF ZoneItem::boundingRect() const
{
    return {QPointF(0, 0), m_size};
}

QPainterPath ZoneItem::shape() const
{
    // The second one is a hole in the first: a path is filled where an odd number of its outlines are around.
    QPainterPath path;
    path.addRect(boundingRect());
    path.addRect(boundingRect().adjusted(EdgeWidth, TitleHeight, -EdgeWidth, -EdgeWidth));
    return path;
}

void ZoneItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    // Its outline one pixel inside its bounds, as the one of a component.
    QPainterPath outline;
    outline.addRoundedRect(boundingRect().adjusted(1, 1, -1, -1), CornerRadius, CornerRadius);
    QPainterPath titleBar;
    titleBar.addRect(QRectF(0, 0, m_size.width(), TitleHeight));
    QColor fill = m_color;
    fill.setAlphaF(FillOpacity);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(m_color, 1));
    painter->setBrush(fill);
    painter->drawPath(outline);
    painter->fillPath(outline.intersected(titleBar), m_color);

    if(m_titleEditor != nullptr && m_titleEditor->isVisible())
    {
        return; // Its title is the text being edited
    }
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(textColorOver(m_color));
    const QRectF title = titleRect();
    painter->drawText(title, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetricsF(font).elidedText(m_title, Qt::ElideRight, title.width()));
}

void ZoneItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    const Qt::Edges edges = edgesAt(event->pos());
    if(!edges)
    {
        unsetCursor();
    }
    else
    {
        setCursor(resizeCursor(edges));
    }
}

void ZoneItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }
    m_resizedEdges = edgesAt(event->pos());
    m_moving = !m_resizedEdges;
    // What is in it now, not what it is then moved over.
    m_followers = m_moving ? content() : QList<QGraphicsItem*>();
    const QRectF bounds = sceneBoundingRect();
    m_grabOffset = event->scenePos() - QPointF(m_resizedEdges.testFlag(Qt::LeftEdge) ? bounds.left() : bounds.right(),
                                               m_resizedEdges.testFlag(Qt::TopEdge) ? bounds.top() : bounds.bottom());
}

void ZoneItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_moving)
    {
        const QPointF offset = event->scenePos() - event->lastScenePos();
        moveBy(offset.x(), offset.y());
        for(QGraphicsItem* follower : std::as_const(m_followers))
        {
            follower->moveBy(offset.x(), offset.y());
        }
    }
    else if(m_resizedEdges != Qt::Edges())
    {
        resize(event->scenePos());
    }
}

void ZoneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_moving = false;
        m_followers.clear();
        m_resizedEdges = {};
    }
}

void ZoneItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && !edgesAt(event->pos()))
    {
        editTitle();
    }
    else
    {
        QGraphicsItem::mouseDoubleClickEvent(event); // A press
    }
}

QRectF ZoneItem::titleRect() const
{
    return {Padding, 0, m_size.width() - 2 * Padding, TitleHeight};
}

Qt::Edges ZoneItem::edgesAt(const QPointF& position) const
{
    const QRectF bounds = boundingRect();
    Qt::Edges edges;
    if(bounds.adjusted(EdgeWidth, EdgeWidth, -EdgeWidth, -EdgeWidth).contains(position))
    {
        return edges;
    }
    if(position.x() < bounds.left() + CornerSize)
    {
        edges |= Qt::LeftEdge;
    }
    else if(position.x() > bounds.right() - CornerSize)
    {
        edges |= Qt::RightEdge;
    }
    if(position.y() < bounds.top() + CornerSize)
    {
        edges |= Qt::TopEdge;
    }
    else if(position.y() > bounds.bottom() - CornerSize)
    {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

QList<QGraphicsItem*> ZoneItem::content() const
{
    const QRectF bounds = sceneBoundingRect();
    QList<QGraphicsItem*> content;
    for(QGraphicsItem* item : scene()->items(bounds, Qt::IntersectsItemBoundingRect))
    {
        const QRectF itemBounds = item->sceneBoundingRect();
        const bool component = qobject_cast<const ComponentItem*>(item->toGraphicsObject()) != nullptr;
        const bool zone = item != this && item->type() == Type;
        if((component && bounds.contains(itemBounds.center())) || (zone && bounds.contains(itemBounds)))
        {
            content.append(item);
        }
    }
    return content;
}

void ZoneItem::resize(const QPointF& scenePosition)
{
    const QPointF edges = scenePosition - m_grabOffset;
    QRectF bounds = sceneBoundingRect();
    if(m_resizedEdges.testFlag(Qt::LeftEdge))
    {
        bounds.setLeft(std::min(edges.x(), bounds.right() - MinimumSize.width()));
    }
    else if(m_resizedEdges.testFlag(Qt::RightEdge))
    {
        bounds.setRight(std::max(edges.x(), bounds.left() + MinimumSize.width()));
    }
    if(m_resizedEdges.testFlag(Qt::TopEdge))
    {
        bounds.setTop(std::min(edges.y(), bounds.bottom() - MinimumSize.height()));
    }
    else if(m_resizedEdges.testFlag(Qt::BottomEdge))
    {
        bounds.setBottom(std::max(edges.y(), bounds.top() + MinimumSize.height()));
    }
    prepareGeometryChange();
    setPos(bounds.topLeft()); // What is in it stays where it is: only dragging its title moves it
    m_size = bounds.size();
    stack();
}

void ZoneItem::stack()
{
    setZValue(-m_size.width() * m_size.height());
}
