// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QStyleOptionGraphicsItem>
#include <utility>

#include "Colors.h"
#include "InterfaceItem.h"
#include "NameEditor.h"
#include "Nebula/Component/Component.h"
#include "Theme.h"

namespace
{

// The names, centered on the position of the item, where the component was dropped; its interfaces are below.
constexpr QRectF Header(-80, -28, 160, 56);
constexpr qreal Padding = 8;
constexpr qreal CornerRadius = 6;
// The mark of its state, in the top right corner of its names.
constexpr qreal StateMark = 8;
// Where the names are written, one above the other.
constexpr QRectF Names = Header.adjusted(Padding, Padding, -Padding, -Padding);
// Its name leaves the corner to the mark, and as much on its left to stay centered.
constexpr qreal MarkWidth = StateMark + Padding / 2;
constexpr QRectF NameRect(Names.left() + MarkWidth, Names.top(), Names.width() - 2 * MarkWidth, Names.height() / 2);
// How much of the color of its name the name of its component keeps over the color of an item, as it is fainter over
// the ones of the style.
constexpr float ComponentNameOpacity = 0.7F;

// What the name of an item of color is written in: of the style when it has none; read over its color otherwise, as
// it shows over the item, which it lets through when it is translucent.
QColor nameColor(const QColor& color, const QPalette& palette)
{
    if(!color.isValid())
    {
        return palette.color(QPalette::ButtonText);
    }
    const QColor background = over(palette.color(QPalette::Button), palette.color(QPalette::Base));
    return textColorOver(over(color, background));
}

} // namespace

ComponentItem::ComponentItem(Nebula::Component& component, QString componentName)
    : m_component(component), m_componentName(std::move(componentName))
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    setToolTip(m_component.componentId());

    connect(&m_component, &Nebula::Component::interfacesChanged, this, &ComponentItem::refreshInterfaces);
    connect(&m_component, &Nebula::Component::stateChanged, this, [this] { update(); }); // Its mark
    connect(&m_component, &QObject::objectNameChanged, this, [this] { update(); });      // Its name
    refreshInterfaces();
}

Nebula::Component& ComponentItem::component() const
{
    return m_component;
}

const QList<InterfaceItem*>& ComponentItem::consumedInterfaces() const
{
    return m_consumedInterfaces;
}

const QList<InterfaceItem*>& ComponentItem::providedInterfaces() const
{
    return m_providedInterfaces;
}

const QColor& ComponentItem::color() const
{
    return m_color;
}

void ComponentItem::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void ComponentItem::editName()
{
    if(m_nameEditor == nullptr)
    {
        m_nameEditor = new NameEditor(*this, [this](const QString& name) { emit nameEdited(name); });
    }
    m_nameEditor->edit(m_component.objectName(), nameColor(m_color, QApplication::palette()), NameRect,
                       Qt::AlignHCenter);
}

void ComponentItem::setEditable(bool editable)
{
    m_editable = editable;
    setFlag(ItemIsMovable, editable);
}

QRectF ComponentItem::boundingRect() const
{
    const qsizetype rows = m_consumedInterfaces.size() + m_providedInterfaces.size();
    return Header.adjusted(0, 0, 0, static_cast<qreal>(rows) * InterfaceItem::RowHeight);
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const bool selected = option->state.testFlag(QStyle::State_Selected);
    // One pixel inside its bounds.
    QPainterPath outline;
    outline.addRoundedRect(boundingRect().adjusted(1, 1, -1, -1), CornerRadius, CornerRadius);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillPath(outline, palette.color(QPalette::Button));
    if(m_color.isValid())
    {
        // Behind its names only: its interfaces keep the colors of the style, which their dots are drawn with.
        QPainterPath header;
        header.addRect(Header);
        painter->fillPath(outline.intersected(header), m_color);
    }
    // Once it is filled, for its color not to cover the half of its outline that is inside.
    painter->setPen(selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(outline);

    const QRectF componentRect(Names.left(), NameRect.bottom(), Names.width(), Names.height() / 2);
    const QColor nameInk = nameColor(m_color, palette);
    QColor componentInk = palette.color(QPalette::PlaceholderText);
    if(m_color.isValid())
    {
        componentInk = nameInk;
        componentInk.setAlphaF(ComponentNameOpacity);
    }

    // A shape, as the dots of its interfaces: a triangle while it runs, nothing while it is edited.
    const QRectF mark(Names.right() - StateMark, Names.top() + (NameRect.height() - StateMark) / 2, StateMark,
                      StateMark);
    painter->setPen(Qt::NoPen);
    painter->setBrush(nameInk);
    if(m_component.state() == Nebula::ComponentState::Running)
    {
        painter->drawPolygon(QPolygonF({mark.topLeft(), mark.bottomLeft(), QPointF(mark.right(), mark.center().y())}));
    }

    const QFont font = painter->font();
    if(!isNameEdited()) // Its name is otherwise the text being edited
    {
        const QFont title = titleFont(font);
        painter->setFont(title);
        painter->setPen(nameInk);
        painter->drawText(NameRect, Qt::AlignCenter,
                          QFontMetricsF(title).elidedText(m_component.objectName(), Qt::ElideRight, NameRect.width()));
    }

    painter->setFont(font);
    painter->setPen(componentInk);
    painter->drawText(componentRect, Qt::AlignCenter,
                      QFontMetricsF(font).elidedText(m_componentName, Qt::ElideRight, componentRect.width()));
}

void ComponentItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(!isNameEdited())
    {
        QGraphicsObject::mouseMoveEvent(event);
    }
}

void ComponentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_editable && event->button() == Qt::LeftButton && Header.contains(event->pos()))
    {
        editName();
    }
    else
    {
        QGraphicsObject::mouseDoubleClickEvent(event); // A press
    }
}

bool ComponentItem::isNameEdited() const
{
    return m_nameEditor != nullptr && m_nameEditor->isVisible();
}

void ComponentItem::refreshInterfaces()
{
    // The rows start in the padding under the names and leave as much under the last one.
    qreal row = Header.bottom() - Padding / 2 + InterfaceItem::RowHeight / 2;
    // items shows interfaces, of kind, down from row, each on the edge at x.
    const auto refresh = [&](QList<InterfaceItem*>& items, const QList<Nebula::InterfaceInfo>& interfaces,
                             InterfaceItem::Kind kind, qreal x)
    {
        for(qsizetype index = 0; index < interfaces.size(); ++index, row += InterfaceItem::RowHeight)
        {
            if(index == items.size())
            {
                prepareGeometryChange(); // A row more
                items.append(new InterfaceItem(kind, interfaces.at(index), Header.width() - 2 * Padding, *this));
            }
            items.at(index)->setAvailable(interfaces.at(index).available);
            items.at(index)->setPos(x, row); // Lower than it was when an interface was declared above it
        }
    };
    // On the edges drawn by paint(), one pixel inside its bounds.
    refresh(m_consumedInterfaces, m_component.consumedInterfaces(), InterfaceItem::Kind::Consumed, Header.left() + 1);
    refresh(m_providedInterfaces, m_component.providedInterfaces(), InterfaceItem::Kind::Provided, Header.right() - 1);
}
