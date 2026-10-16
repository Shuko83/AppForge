// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "DashedLinkItem.h"

#include <QApplication>
#include <QGraphicsObject>
#include <QPainter>
#include <QPalette>
#include <QWidget>
#include <utility>

namespace
{

constexpr qreal LineWidth = 1;

} // namespace

DashedLinkItem::DashedLinkItem(QGraphicsObject& from, QGraphicsObject& to, std::function<void(DashedLinkItem&)> removed)
    : m_from(from), m_to(&to), m_removed(std::move(removed))
{
    setZValue(-1); // Under the items it joins, as the links of interfaces are
    follow();
}

DashedLinkItem::~DashedLinkItem()
{
    m_removed(*this);
}

void DashedLinkItem::follow()
{
    if(m_to == nullptr) // Gone before the item it joins: it is about to go too
    {
        return;
    }
    prepareGeometryChange();
    m_line = QLineF(m_from.scenePos(), m_to->scenePos());
}

QRectF DashedLinkItem::boundingRect() const
{
    return QRectF(m_line.p1(), m_line.p2()).normalized().adjusted(-LineWidth, -LineWidth, LineWidth, LineWidth);
}

void DashedLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(QPalette::Mid), LineWidth, Qt::DashLine));
    painter->drawLine(m_line);
}
