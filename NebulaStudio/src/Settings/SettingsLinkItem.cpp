// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsLinkItem.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QWidget>

#include "ComponentItem.h"
#include "Settings/SettingsItem.h"

namespace
{

constexpr qreal LineWidth = 1;

} // namespace

SettingsLinkItem::SettingsLinkItem(SettingsItem& settings, ComponentItem& component)
    : m_settings(settings), m_component(&component)
{
    setZValue(-1); // Under the items it joins, as the links of interfaces are
    follow();
}

SettingsLinkItem::~SettingsLinkItem()
{
    m_settings.removeLink(*this);
}

ComponentItem* SettingsLinkItem::component() const
{
    return m_component;
}

void SettingsLinkItem::follow()
{
    if(m_component == nullptr) // Gone before its component: it is about to go too
    {
        return;
    }
    prepareGeometryChange();
    m_line = QLineF(m_settings.scenePos(), m_component->scenePos());
}

QRectF SettingsLinkItem::boundingRect() const
{
    return QRectF(m_line.p1(), m_line.p2()).normalized().adjusted(-LineWidth, -LineWidth, LineWidth, LineWidth);
}

void SettingsLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(palette.color(QPalette::Mid), LineWidth, Qt::DashLine));
    painter->drawLine(m_line);
}
