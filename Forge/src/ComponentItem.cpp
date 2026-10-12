#include "ComponentItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QPolygonF>
#include <QStyleOptionGraphicsItem>
#include <utility>

#include "AppForge/Component/Component.h"
#include "InterfaceItem.h"

namespace
{

// The names, centered on the position of the item, where the component was dropped; its interfaces are below.
constexpr QRectF Header(-80, -28, 160, 56);
constexpr qreal Padding = 8;
// The mark of its state, in the top right corner of its names.
constexpr qreal StateMark = 8;

} // namespace

ComponentItem::ComponentItem(AppForge::Component& component, QString componentName)
    : m_component(component), m_componentName(std::move(componentName))
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    setToolTip(m_component.componentId());

    connect(&m_component, &AppForge::Component::interfacesChanged, this, &ComponentItem::refreshInterfaces);
    connect(&m_component, &AppForge::Component::stateChanged, this, [this] { update(); }); // Its mark
    refreshInterfaces();
}

AppForge::Component& ComponentItem::component() const
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

QRectF ComponentItem::boundingRect() const
{
    const qsizetype rows = m_consumedInterfaces.size() + m_providedInterfaces.size();
    return Header.adjusted(0, 0, 0, static_cast<qreal>(rows) * InterfaceItem::RowHeight);
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const bool selected = option->state.testFlag(QStyle::State_Selected);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1));
    painter->setBrush(palette.color(QPalette::Button));
    painter->drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1), 6, 6);

    const QRectF text = Header.adjusted(Padding, Padding, -Padding, -Padding);
    // Its name leaves the corner to the mark, and as much on its left to stay centered.
    const qreal markWidth = StateMark + Padding / 2;
    const QRectF nameRect(text.left() + markWidth, text.top(), text.width() - 2 * markWidth, text.height() / 2);
    const QRectF componentRect(text.left(), nameRect.bottom(), text.width(), text.height() / 2);

    // Shapes, as the dots of its interfaces: nothing until it is initialized, then a square, and a triangle while it
    // runs.
    const QRectF mark(text.right() - StateMark, text.top() + (nameRect.height() - StateMark) / 2, StateMark, StateMark);
    painter->setPen(Qt::NoPen);
    painter->setBrush(palette.color(QPalette::ButtonText));
    if(m_component.state() == AppForge::ComponentState::Running)
    {
        painter->drawPolygon(QPolygonF({mark.topLeft(), mark.bottomLeft(), QPointF(mark.right(), mark.center().y())}));
    }
    else if(m_component.state() == AppForge::ComponentState::Ready)
    {
        painter->drawRect(mark);
    }

    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(palette.color(QPalette::ButtonText));
    painter->drawText(nameRect, Qt::AlignCenter,
                      QFontMetricsF(font).elidedText(m_component.objectName(), Qt::ElideRight, nameRect.width()));

    font.setBold(false);
    painter->setFont(font);
    painter->setPen(palette.color(QPalette::PlaceholderText));
    painter->drawText(componentRect, Qt::AlignCenter,
                      QFontMetricsF(font).elidedText(m_componentName, Qt::ElideRight, componentRect.width()));
}

void ComponentItem::refreshInterfaces()
{
    // The rows start in the padding under the names and leave as much under the last one.
    qreal row = Header.bottom() - Padding / 2 + InterfaceItem::RowHeight / 2;
    // items shows interfaces, of kind, down from row, each on the edge at x.
    const auto refresh = [&](QList<InterfaceItem*>& items, const QList<AppForge::InterfaceInfo>& interfaces,
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
