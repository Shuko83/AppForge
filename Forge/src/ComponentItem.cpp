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

    for(const auto changed : {&AppForge::Component::interfaceAdded, &AppForge::Component::interfaceAvailable,
                              &AppForge::Component::interfaceUnavailable})
    {
        connect(&m_component, changed, this, &ComponentItem::refreshInterfaces);
    }
    refreshInterfaces();
}

AppForge::Component& ComponentItem::component() const
{
    return m_component;
}

QRectF ComponentItem::boundingRect() const
{
    return Header.adjusted(0, 0, 0, static_cast<qreal>(m_interfaces.size()) * InterfaceItem::RowHeight);
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
    const QList<AppForge::InterfaceInfo> interfaces = m_component.providedInterfaces();
    for(qsizetype index = 0; index < interfaces.size(); ++index)
    {
        if(index < m_interfaces.size())
        {
            m_interfaces.at(index)->setAvailable(interfaces.at(index).available);
            continue;
        }
        prepareGeometryChange(); // A row more
        auto* item = new InterfaceItem(interfaces.at(index), Header.width() - 2 * Padding, this);
        // On the edge drawn by paint(), one pixel inside its bounds; the rows start in the padding under the names
        // and leave as much under the last one.
        item->setPos(Header.right() - 1,
                     Header.bottom() - Padding / 2 + (static_cast<qreal>(index) + 0.5) * InterfaceItem::RowHeight);
        m_interfaces.append(item);
    }
}
