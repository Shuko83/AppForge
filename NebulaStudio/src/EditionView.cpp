// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionView.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace
{

// The whole scene fits in a window at the smallest zoom level; the largest one is beyond what is read comfortably.
constexpr qreal MinimumZoom = 0.25;
constexpr qreal MaximumZoom = 4;
// What a key, or a notch of the wheel, multiplies or divides the zoom by.
constexpr qreal ZoomStep = 1.25;
// The angle a notch of the wheel turns, in eighths of a degree.
constexpr qreal WheelNotch = 120;

} // namespace

EditionView::EditionView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent) {}

qreal EditionView::zoom() const
{
    return transform().m11();
}

void EditionView::setZoom(qreal zoom)
{
    zoom = std::clamp(zoom, MinimumZoom, MaximumZoom);
    if(qFuzzyCompare(zoom, this->zoom()))
    {
        return;
    }
    setTransform(QTransform::fromScale(zoom, zoom));
    emit zoomChanged(zoom);
}

void EditionView::zoomIn()
{
    setZoom(zoom() * ZoomStep);
}

void EditionView::zoomOut()
{
    setZoom(zoom() / ZoomStep);
}

void EditionView::resetZoom()
{
    setZoom(1);
}

void EditionView::wheelEvent(QWheelEvent* event)
{
    if(!event->modifiers().testFlag(Qt::ControlModifier))
    {
        QGraphicsView::wheelEvent(event);
        return;
    }
    // What is under the cursor stays there, the view scrolled by how far it went.
    const QPoint cursor = event->position().toPoint();
    const QPointF under = mapToScene(cursor);
    setZoom(zoom() * std::pow(ZoomStep, event->angleDelta().y() / WheelNotch));
    const QPoint moved = mapFromScene(under) - cursor;
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + moved.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() + moved.y());
    event->accept();
}

void EditionView::keyPressEvent(QKeyEvent* event)
{
    // The scene first: a name edited there takes + and - as any other character.
    QGraphicsView::keyPressEvent(event);
    if(event->isAccepted() ||
       (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) != Qt::NoModifier)
    {
        return;
    }
    switch(event->key())
    {
    case Qt::Key_Plus:
        zoomIn();
        event->accept();
        break;
    case Qt::Key_Minus:
        zoomOut();
        event->accept();
        break;
    default:
        break;
    }
}
