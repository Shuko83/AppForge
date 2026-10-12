#include "EditionScene.h"

#include <QColorDialog>
#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QList>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QToolTip>
#include <array>
#include <cmath>
#include <optional>

#include "Assembly.h"
#include "Component/ComponentFactory.h"
#include "ComponentItem.h"
#include "ComponentModel.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
#include "ZoneItem.h"

namespace
{

constexpr qreal GridStep = 20;
// Large enough not to grow, so that the view does not move when a component is dropped.
constexpr QRectF SceneRect(0, 0, 4000, 3000);
// Where the hint is drawn while nothing is in the edition zone, in its top left corner.
constexpr QRectF HintRect(GridStep, GridStep, 600, 2 * GridStep);
// A zone as it is added: room for a few components, until it is resized.
constexpr QSizeF ZoneSize(16 * GridStep, 10 * GridStep);
// The colors the zones are added with, in turn, which tells them apart until theirs is chosen.
constexpr std::array<QRgb, 6> ZoneColors = {0x4E79A7, 0xF28E2B, 0x59A14F, 0xE15759, 0xB07AA1, 0x76B7B2};

// Accepts a drag carrying a component, as a copy: the component stays in its list.
void acceptComponent(QGraphicsSceneDragDropEvent& event)
{
    if(event.mimeData()->hasFormat(ComponentModel::MimeType))
    {
        event.setDropAction(Qt::CopyAction);
        event.accept();
    }
    else
    {
        event.ignore();
    }
}

} // namespace

EditionScene::EditionScene(Assembly& assembly, const AppForge::ComponentFactory& factory, QObject* parent)
    : QGraphicsScene(SceneRect, parent), m_assembly(assembly), m_factory(factory)
{
}

AppForge::Component* EditionScene::selectedComponent() const
{
    // Only the components can be selected.
    const QList<QGraphicsItem*> selected = selectedItems();
    const auto* item =
        selected.size() == 1 ? qobject_cast<const ComponentItem*>(selected.constFirst()->toGraphicsObject()) : nullptr;
    return item != nullptr ? &item->component() : nullptr;
}

void EditionScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event);
}

void EditionScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event);
}

void EditionScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    const QString componentId = QString::fromUtf8(event->mimeData()->data(ComponentModel::MimeType));
    const std::optional<AppForge::ComponentInfo> info = m_factory.component(componentId);
    AppForge::Component* component = info ? m_assembly.instantiate(componentId) : nullptr;
    if(component == nullptr)
    {
        event->ignore();
        return;
    }
    auto* item = new ComponentItem(*component, info->name);
    item->setPos(event->scenePos());
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    clearSelection();
    item->setSelected(true);
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void EditionScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    InterfaceItem* source = event->button() == Qt::LeftButton ? interfaceAt(event->scenePos()) : nullptr;
    if(source == nullptr)
    {
        QGraphicsScene::mousePressEvent(event);
        return;
    }
    // Not passed to the items: its component would move.
    m_linkSource = source;
    m_pendingLink = addPath({}, QPen(palette().color(QPalette::ButtonText), 1.5, Qt::DashLine));
    m_pendingLink->setZValue(1); // Over the components it is drawn across
    dimLinkTargets(source);
    event->accept();
}

void EditionScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_linkSource == nullptr)
    {
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }
    // What the dot under the cursor is: no tooltip shows by itself while a button is pressed, and its type is what
    // tells whether the link can end there.
    const InterfaceItem* hovered = interfaceAt(event->scenePos());
    QToolTip::showText(event->screenPos(),
                       hovered != nullptr && hovered != m_linkSource ? hovered->toolTip() : QString(), event->widget());
    // To the dot it would be bound to rather than to the cursor, which tells that releasing there binds them.
    const InterfaceItem* target = hovered != nullptr && m_linkSource->canLinkTo(*hovered) ? hovered : nullptr;
    const QPointF end = target != nullptr ? target->scenePos() : event->scenePos();
    const QPointF start = m_linkSource->scenePos();
    const bool provided = m_linkSource->kind() == InterfaceItem::Kind::Provided;
    m_pendingLink->setPath(provided ? LinkItem::curve(start, end) : LinkItem::curve(end, start));
    event->accept();
}

void EditionScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if(m_linkSource == nullptr || event->button() != Qt::LeftButton)
    {
        QGraphicsScene::mouseReleaseEvent(event);
        return;
    }
    InterfaceItem* source = m_linkSource;
    InterfaceItem* target = linkTargetAt(event->scenePos());
    m_linkSource = nullptr;
    delete m_pendingLink;
    m_pendingLink = nullptr;
    dimLinkTargets(nullptr);
    QToolTip::hideText();
    if(target != nullptr)
    {
        link(*source, *target);
    }
    event->accept();
}

void EditionScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Not while a link is drawn or an item is dragged: what is dragged could be removed from under the cursor.
    if(m_linkSource != nullptr || mouseGrabberItem() != nullptr)
    {
        return;
    }
    ZoneItem* zone = zoneAt(event->scenePos());
    QMenu menu(event->widget());
    const QAction* add = menu.addAction(tr("Add zone"));
    const QAction* rename = nullptr;
    const QAction* recolor = nullptr;
    const QAction* remove = nullptr;
    if(zone != nullptr)
    {
        menu.addSeparator();
        rename = menu.addAction(tr("Rename zone"));
        recolor = menu.addAction(tr("Change zone color..."));
        remove = menu.addAction(tr("Remove zone"));
    }
    // Done once the menu is closed, and the focus back in the view: editing a title takes it.
    const QAction* chosen = menu.exec(event->screenPos());
    event->accept();
    if(chosen == nullptr)
    {
        return;
    }
    if(chosen == add)
    {
        addZone(event->scenePos());
    }
    else if(chosen == rename)
    {
        zone->editTitle();
    }
    else if(chosen == recolor)
    {
        const QColor color = QColorDialog::getColor(zone->color(), event->widget(), tr("Zone color"));
        if(color.isValid())
        {
            zone->setColor(color);
        }
    }
    else if(chosen == remove)
    {
        delete zone; // Not what is in it
        update(HintRect);
    }
}

InterfaceItem* EditionScene::interfaceAt(const QPointF& position) const
{
    for(QGraphicsItem* item : items(position))
    {
        if(auto* interfaceItem = qgraphicsitem_cast<InterfaceItem*>(item))
        {
            return interfaceItem;
        }
    }
    return nullptr;
}

InterfaceItem* EditionScene::linkTargetAt(const QPointF& position) const
{
    InterfaceItem* target = interfaceAt(position);
    return target != nullptr && m_linkSource != nullptr && m_linkSource->canLinkTo(*target) ? target : nullptr;
}

void EditionScene::dimLinkTargets(const InterfaceItem* source)
{
    for(QGraphicsItem* item : items())
    {
        if(auto* interfaceItem = qgraphicsitem_cast<InterfaceItem*>(item))
        {
            interfaceItem->setDimmed(source != nullptr && interfaceItem != source &&
                                     !source->canLinkTo(*interfaceItem));
        }
    }
}

void EditionScene::link(InterfaceItem& source, InterfaceItem& target)
{
    const bool sourceProvided = source.kind() == InterfaceItem::Kind::Provided;
    InterfaceItem& provided = sourceProvided ? source : target;
    InterfaceItem& consumed = sourceProvided ? target : source;
    if(!consumed.component().bindInterface(consumed.name(), provided.component(), provided.name()))
    {
        return;
    }
    // What a component consumes is bound to one interface: the new one.
    while(!consumed.links().isEmpty())
    {
        delete consumed.links().constFirst();
    }
    addItem(new LinkItem(provided, consumed));
}

ZoneItem* EditionScene::zoneAt(const QPointF& position) const
{
    // By their bounds, as their shape is only their title and their edges; the topmost one is the smallest.
    for(QGraphicsItem* item : items(position, Qt::IntersectsItemBoundingRect))
    {
        if(auto* zone = qgraphicsitem_cast<ZoneItem*>(item))
        {
            return zone;
        }
    }
    return nullptr;
}

void EditionScene::addZone(const QPointF& position)
{
    auto* zone = new ZoneItem(ZoneSize, tr("Zone"), QColor(ZoneColors.at(m_zoneCount++ % ZoneColors.size())));
    zone->setPos(position);
    addItem(zone);
    update(HintRect); // Hidden once anything is in the edition zone
    zone->editTitle();
}

void EditionScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsScene::drawBackground(painter, rect);
    painter->save();

    QColor lineColor = palette().color(QPalette::Mid);
    lineColor.setAlphaF(0.25F);
    painter->setPen(QPen(lineColor, 0));
    QList<QLineF> lines;
    for(qreal x = std::floor(rect.left() / GridStep) * GridStep; x <= rect.right(); x += GridStep)
    {
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for(qreal y = std::floor(rect.top() / GridStep) * GridStep; y <= rect.bottom(); y += GridStep)
    {
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    painter->drawLines(lines);

    if(items().isEmpty())
    {
        painter->setPen(palette().color(QPalette::PlaceholderText));
        painter->drawText(HintRect, Qt::AlignLeft | Qt::AlignVCenter,
                          tr("Drag a component from the list of components and drop it here to instantiate it\n"
                             "Right-click to add a zone, which groups components"));
    }
    painter->restore();
}
