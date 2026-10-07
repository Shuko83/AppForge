#include "ComponentItem.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <utility>

#include "Component/Component.h"

namespace
{

// Centered on the position of the item, where the component was dropped.
constexpr QRectF Bounds(-80, -28, 160, 56);
constexpr qreal Padding = 8;

} // namespace

ComponentItem::ComponentItem(AppForge::Component& component, QString componentName)
    : m_component(component), m_componentName(std::move(componentName))
{
    setFlags(ItemIsMovable | ItemIsSelectable);
    setToolTip(m_component.componentId());
}

AppForge::Component& ComponentItem::component() const
{
    return m_component;
}

QRectF ComponentItem::boundingRect() const
{
    return Bounds;
}

void ComponentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const QPalette palette = widget != nullptr ? widget->palette() : QApplication::palette();
    const bool selected = option->state.testFlag(QStyle::State_Selected);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(selected ? QPen(palette.color(QPalette::Highlight), 2) : QPen(palette.color(QPalette::Mid), 1));
    painter->setBrush(palette.color(QPalette::Button));
    painter->drawRoundedRect(Bounds.adjusted(1, 1, -1, -1), 6, 6);

    const QRectF text = Bounds.adjusted(Padding, Padding, -Padding, -Padding);
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
