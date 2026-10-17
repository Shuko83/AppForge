// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SettingsItem.h"

#include <QApplication>
#include <QDir>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStringList>
#include <QStyleOptionGraphicsItem>
#include <algorithm>

#include "ComponentItem.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Settings/SettingsLinkItem.h"
#include "Theme.h"

namespace
{

// The sheet, centered on the position of the item, of the size of the names of a component.
constexpr QRectF Sheet(-80, -28, 160, 56);
constexpr qreal Padding = 8;
// The side of its folded corner, its top right one.
constexpr qreal Fold = 12;
// Where the name of its file is written, above "Settings file", clear of the fold.
constexpr QRectF Names = Sheet.adjusted(Padding, Padding, -Padding, -Padding);
constexpr QRectF NameRect(Names.left() + Fold, Names.top(), Names.width() - 2 * Fold, Names.height() / 2);
constexpr QRectF KindRect(Names.left(), NameRect.bottom(), Names.width(), Names.height() / 2);

// The property of entry as its tooltip lists it: <component>.<property>, with the name of its object between them
// when it is of a named one.
QString entryName(const Nebula::SettingsEntry& entry)
{
    QString name = entry.component->objectName() + u'.';
    const QList<Nebula::PropertyObject> objects = entry.component->propertyObjects();
    const auto object = std::ranges::find(objects, entry.object, &Nebula::PropertyObject::object);
    if(object != objects.end() && !object->name.isEmpty())
    {
        name += object->name + u'.';
    }
    return name + entry.property;
}

} // namespace

SettingsItem::SettingsItem(Nebula::SettingsFile& file) : m_file(file)
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    connect(&m_file, &Nebula::SettingsFile::changed, this,
            [this]
            {
                refreshLinks();
                refreshToolTip();
                update(); // Its name
            });
    connect(this, &QGraphicsObject::xChanged, this, &SettingsItem::followLinks);
    connect(this, &QGraphicsObject::yChanged, this, &SettingsItem::followLinks);
    refreshToolTip();
}

SettingsItem::~SettingsItem()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst(); // Which removes it
    }
}

Nebula::SettingsFile& SettingsItem::file() const
{
    return m_file;
}

void SettingsItem::removeLink(SettingsLinkItem& link)
{
    m_links.removeOne(&link);
}

QRectF SettingsItem::boundingRect() const
{
    return Sheet;
}

void SettingsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    // One pixel inside its bounds, as a component.
    const QRectF sheet = Sheet.adjusted(1, 1, -1, -1);
    const QPointF foldStart(sheet.right() - Fold, sheet.top());
    const QPointF foldEnd(sheet.right(), sheet.top() + Fold);
    QPainterPath outline(sheet.topLeft());
    outline.lineTo(foldStart);
    outline.lineTo(foldEnd);
    outline.lineTo(sheet.bottomRight());
    outline.lineTo(sheet.bottomLeft());
    outline.closeSubpath();

    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillPath(outline, palette.color(QPalette::Base));
    painter->setPen(selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(outline);
    painter->drawPolyline(QPolygonF({foldStart, QPointF(foldStart.x(), foldEnd.y()), foldEnd}));

    const QFont font = painter->font();
    const QFont title = titleFont(font);
    painter->setFont(title);
    painter->setPen(palette.color(QPalette::Text));
    painter->drawText(NameRect, Qt::AlignCenter,
                      QFontMetricsF(title).elidedText(m_file.fileName(), Qt::ElideMiddle, NameRect.width()));
    painter->setFont(font);
    painter->setPen(palette.color(QPalette::PlaceholderText));
    painter->drawText(KindRect, Qt::AlignCenter, tr("Settings file"));
}

QVariant SettingsItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if(change == ItemSceneHasChanged)
    {
        refreshLinks();
    }
    return QGraphicsObject::itemChange(change, value);
}

void SettingsItem::refreshLinks()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst();
    }
    if(scene() == nullptr)
    {
        return;
    }
    QList<const Nebula::Component*> held; // The components it holds properties of
    for(const Nebula::SettingsEntry& entry : m_file.entries())
    {
        held.append(entry.component);
    }
    const QList<QGraphicsItem*> items = scene()->items();
    for(QGraphicsItem* item : items)
    {
        auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(component == nullptr || !held.contains(&component->component()))
        {
            continue;
        }
        // As it moves, as it does too.
        connect(component, &QGraphicsObject::xChanged, this, &SettingsItem::followLinks, Qt::UniqueConnection);
        connect(component, &QGraphicsObject::yChanged, this, &SettingsItem::followLinks, Qt::UniqueConnection);
        auto* link = new SettingsLinkItem(*this, *component);
        m_links.append(link);
        scene()->addItem(link);
    }
}

void SettingsItem::followLinks()
{
    for(SettingsLinkItem* link : std::as_const(m_links))
    {
        link->follow();
    }
}

void SettingsItem::refreshToolTip()
{
    QStringList lines{QDir::toNativeSeparators(m_file.filePath())};
    for(const Nebula::SettingsEntry& entry : m_file.entries())
    {
        lines.append(entryName(entry));
    }
    if(m_file.entries().isEmpty())
    {
        lines.append(tr("No property yet: double-click to choose those it holds"));
    }
    setToolTip(lines.join(u'\n'));
}
