// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ExposureItem.h"

#include <QApplication>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <algorithm>
#include <memory>

#include "AppForge/Composite/Composite.h"
#include "AppForge/Composite/Gateway.h"
#include "ComponentItem.h"
#include "DashedLinkItem.h"
#include "InterfaceItem.h"

namespace
{

// Its names, centered on its position, as those of a component; its rows are below.
constexpr QRectF Header(-100, -28, 200, 56);
constexpr qreal Padding = 8;
constexpr qreal CornerRadius = 6;
constexpr QRectF Names = Header.adjusted(Padding, Padding, -Padding, -Padding);
constexpr QRectF NameRect(Names.left(), Names.top(), Names.width(), Names.height() / 2);
constexpr QRectF KindRect(Names.left(), NameRect.bottom(), Names.width(), Names.height() / 2);
// Where the first row is centered: in the padding under the names, as the rows of a component.
constexpr qreal FirstRow = Header.bottom() - Padding / 2 + InterfaceItem::RowHeight / 2;

} // namespace

ExposureItem::ExposureItem(AppForge::Composite& composite) : m_composite(composite)
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    connect(&m_composite, &AppForge::Component::interfacesChanged, this, &ExposureItem::refresh);
    connect(&m_composite, &AppForge::Composite::exposedPropertiesChanged, this, &ExposureItem::refresh);
    connect(this, &QGraphicsObject::xChanged, this, &ExposureItem::followLinks);
    connect(this, &QGraphicsObject::yChanged, this, &ExposureItem::followLinks);
    refresh();
}

ExposureItem::~ExposureItem()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst(); // Which removes it
    }
}

AppForge::Composite& ExposureItem::composite() const
{
    return m_composite;
}

const QList<InterfaceItem*>& ExposureItem::consumedInterfaces() const
{
    return m_consumedInterfaces;
}

const QList<InterfaceItem*>& ExposureItem::providedInterfaces() const
{
    return m_providedInterfaces;
}

QRectF ExposureItem::boundingRect() const
{
    // A row that says nothing is exposed, when nothing is.
    const qsizetype rows =
        std::max<qsizetype>(1, m_consumedInterfaces.size() + m_providedInterfaces.size() + m_properties.size());
    return Header.adjusted(0, 0, 0, static_cast<qreal>(rows) * InterfaceItem::RowHeight);
}

void ExposureItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    QPainterPath outline;
    outline.addRoundedRect(boundingRect().adjusted(1, 1, -1, -1), CornerRadius, CornerRadius);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillPath(outline, palette.color(QPalette::Button));
    // Dashed: the edge of the composite, rather than one of its components.
    QPen pen = selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(outline);

    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(palette.color(QPalette::ButtonText));
    painter->drawText(NameRect, Qt::AlignCenter,
                      QFontMetricsF(font).elidedText(QFileInfo(m_composite.filePath()).completeBaseName(),
                                                     Qt::ElideRight, NameRect.width()));
    font.setBold(false);
    painter->setFont(font);
    painter->setPen(palette.color(QPalette::PlaceholderText));
    painter->drawText(KindRect, Qt::AlignCenter, tr("Exposed"));

    // Under the dots of the gateways, its properties: their names, and where they are.
    const QFontMetricsF metrics(font);
    qreal row = FirstRow + static_cast<qreal>(m_consumedInterfaces.size() + m_providedInterfaces.size()) *
                               InterfaceItem::RowHeight;
    if(m_consumedInterfaces.isEmpty() && m_providedInterfaces.isEmpty() && m_properties.isEmpty())
    {
        const QRectF rect(Names.left(), row - InterfaceItem::RowHeight / 2, Names.width(), InterfaceItem::RowHeight);
        painter->drawText(rect, Qt::AlignCenter,
                          metrics.elidedText(tr("Double-click to expose"), Qt::ElideRight, rect.width()));
        return;
    }
    for(const auto& [name, where] : std::as_const(m_properties))
    {
        const QRectF rect(Names.left(), row - InterfaceItem::RowHeight / 2, Names.width(), InterfaceItem::RowHeight);
        painter->setPen(palette.color(QPalette::ButtonText));
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter,
                          metrics.elidedText(name, Qt::ElideRight, rect.width() / 2));
        painter->setPen(palette.color(QPalette::PlaceholderText));
        painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter,
                          metrics.elidedText(where, Qt::ElideLeft, rect.width() / 2));
        row += InterfaceItem::RowHeight;
    }
}

void ExposureItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if(event->button() == Qt::LeftButton)
    {
        emit editRequested();
        event->accept();
        return;
    }
    QGraphicsObject::mouseDoubleClickEvent(event);
}

QVariant ExposureItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if(change == ItemSceneHasChanged)
    {
        refreshLinks();
    }
    return QGraphicsObject::itemChange(change, value);
}

void ExposureItem::refresh()
{
    // The dots of the gateways, as they are now: what each passes inside the composite.
    struct Dot
    {
        AppForge::Gateway* gateway;
        AppForge::InterfaceInfo info;
    };
    QList<Dot> consumed; // Those the composite provides
    QList<Dot> provided; // Those the composite consumes
    for(const std::unique_ptr<AppForge::Component>& component : m_composite.components())
    {
        auto* gateway = qobject_cast<AppForge::Gateway*>(component.get());
        if(gateway == nullptr)
        {
            continue;
        }
        const bool providedByComposite = gateway->kind() == AppForge::GatewayKind::Provided;
        const QList<AppForge::InterfaceInfo> own =
            providedByComposite ? gateway->consumedInterfaces() : gateway->providedInterfaces();
        if(!own.isEmpty())
        {
            (providedByComposite ? consumed : provided).append(Dot{.gateway = gateway, .info = own.constFirst()});
        }
    }
    // Shown again when one was added, renamed or removed: only their state changes otherwise.
    const auto showing = [](const QList<InterfaceItem*>& items, const QList<Dot>& dots)
    {
        return items.size() == dots.size() &&
               std::ranges::equal(
                   items, dots, [](const InterfaceItem* item, const Dot& dot)
                   { return &item->component() == dot.gateway && item->label() == dot.gateway->objectName(); });
    };
    const bool rebuilt = !showing(m_consumedInterfaces, consumed) || !showing(m_providedInterfaces, provided);
    if(rebuilt)
    {
        qDeleteAll(m_consumedInterfaces); // With their links
        qDeleteAll(m_providedInterfaces);
        m_consumedInterfaces.clear();
        m_providedInterfaces.clear();
    }

    prepareGeometryChange(); // Its rows can be more or fewer
    qreal row = FirstRow;
    // items shows dots, of kind, down from row, each on the edge at x.
    const auto place = [&](QList<InterfaceItem*>& items, const QList<Dot>& dots, InterfaceItem::Kind kind, qreal x)
    {
        for(qsizetype index = 0; index < dots.size(); ++index, row += InterfaceItem::RowHeight)
        {
            AppForge::Gateway* gateway = dots.at(index).gateway;
            if(rebuilt)
            {
                items.append(new InterfaceItem(kind, *gateway, dots.at(index).info, Header.width() - 2 * Padding, *this,
                                               gateway->objectName()));
                // Renamed after the interface of the composite is: shown again then. Its dot goes before it does.
                connect(gateway, &QObject::objectNameChanged, this, &ExposureItem::refresh, Qt::UniqueConnection);
                connect(gateway, &QObject::destroyed, this,
                        [this](QObject* gone)
                        {
                            for(QList<InterfaceItem*>* items : {&m_consumedInterfaces, &m_providedInterfaces})
                            {
                                const auto dot = std::ranges::find_if(
                                    *items, [gone](const InterfaceItem* item)
                                    { return static_cast<const QObject*>(&item->component()) == gone; });
                                if(dot != items->end())
                                {
                                    delete *dot;
                                    items->erase(dot);
                                }
                            }
                            // Once the composite no longer has it among its components.
                            QMetaObject::invokeMethod(this, &ExposureItem::refresh, Qt::QueuedConnection);
                        });
            }
            items.at(index)->setAvailable(dots.at(index).info.available);
            items.at(index)->setPos(x, row);
        }
    };
    place(m_consumedInterfaces, consumed, InterfaceItem::Kind::Consumed, Header.left() + 1);
    place(m_providedInterfaces, provided, InterfaceItem::Kind::Provided, Header.right() - 1);

    m_properties.clear();
    QStringList lines{tr("What %1 shows to the scene it is placed in: double-click to choose it")
                          .arg(QFileInfo(m_composite.filePath()).completeBaseName())};
    for(const AppForge::ExposedProperty& exposed : m_composite.exposedProperties())
    {
        const QString where = exposed.object && !exposed.object->isEmpty()
                                  ? QStringLiteral("%1.%2").arg(exposed.component, *exposed.object)
                                  : exposed.component;
        m_properties.append({exposed.name, where});
        lines.append(tr("%1: %2 of %3").arg(exposed.name, exposed.property, where));
    }
    setToolTip(lines.join(u'\n'));
    update();
    refreshLinks();
    if(rebuilt)
    {
        emit interfacesRebuilt();
    }
}

void ExposureItem::refreshLinks()
{
    while(!m_links.isEmpty())
    {
        delete m_links.constFirst();
    }
    if(scene() == nullptr)
    {
        return;
    }
    QStringList exposing; // The components of the composite it exposes properties of
    for(const AppForge::ExposedProperty& exposed : m_composite.exposedProperties())
    {
        exposing.append(exposed.component);
    }
    const QList<QGraphicsItem*> items = scene()->items();
    for(QGraphicsItem* item : items)
    {
        auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(component == nullptr || !exposing.contains(component->component().objectName()))
        {
            continue;
        }
        connect(component, &QGraphicsObject::xChanged, this, &ExposureItem::followLinks, Qt::UniqueConnection);
        connect(component, &QGraphicsObject::yChanged, this, &ExposureItem::followLinks, Qt::UniqueConnection);
        auto* link = new DashedLinkItem(*this, *component, [this](DashedLinkItem& gone) { m_links.removeOne(&gone); });
        m_links.append(link);
        scene()->addItem(link);
    }
}

void ExposureItem::followLinks()
{
    for(DashedLinkItem* link : std::as_const(m_links))
    {
        link->follow();
    }
}
