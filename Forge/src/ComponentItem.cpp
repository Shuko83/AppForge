#include "ComponentItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <utility>

#include "Component/Component.h"
#include "InterfaceItem.h"

namespace
{

// The names, centered on the position of the item, where the component was dropped; its interfaces are below.
constexpr QRectF Header(-80, -28, 160, 56);
constexpr qreal Padding = 8;

} // namespace

ComponentItem::ComponentItem(AppForge::Component& component, QString componentName)
    : m_component(component), m_componentName(std::move(componentName))
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    setToolTip(m_component.componentId());

    connect(&m_component, &AppForge::Component::interfacesChanged, this, &ComponentItem::refreshInterfaces);
    refreshInterfaces();
}

AppForge::Component& ComponentItem::component() const
{
    return m_component;
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
    const QRectF nameRect(text.left(), text.top(), text.width(), text.height() / 2);
    const QRectF componentRect(text.left(), nameRect.bottom(), text.width(), text.height() / 2);

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
