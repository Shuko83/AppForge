#include "EditionScene.h"

#include <QColorDialog>
#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QHash>
#include <QJsonArray>
#include <QList>
#include <QMenu>
#include <QMetaProperty>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QToolTip>
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

#include "AppForge/Component/ComponentFactory.h"
#include "Assembly.h"
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

// Where an item is, as the "x" and "y" of what a project file holds about it.
void writePosition(QJsonObject& json, const QPointF& position)
{
    json.insert(u"x", position.x());
    json.insert(u"y", position.y());
}

QPointF readPosition(const QJsonObject& json)
{
    return {json.value(u"x").toDouble(), json.value(u"y").toDouble()};
}

// value as a project file holds it; nullopt when it would not be read back as it is, its type being one JSON has
// nothing for.
std::optional<QJsonValue> toJson(const QVariant& value)
{
    const QJsonValue json = QJsonValue::fromVariant(value);
    QVariant read = json.toVariant();
    if(!read.convert(value.metaType()) || read != value)
    {
        return std::nullopt;
    }
    return json;
}

// The properties of component a project file holds, by name: those of properties that can be written.
QJsonObject writableProperties(const AppForge::Component& component, const QList<QMetaProperty>& properties)
{
    QJsonObject json;
    for(const QMetaProperty& property : properties)
    {
        const std::optional<QJsonValue> value =
            property.isWritable() ? toJson(property.read(&component)) : std::nullopt;
        if(value)
        {
            json.insert(QLatin1StringView(property.name()), *value);
        }
    }
    return json;
}

// Sets the property name of component, one of properties, to value; false when it is none of them or cannot be set.
bool writeProperty(AppForge::Component& component, const QList<QMetaProperty>& properties, const QString& name,
                   const QJsonValue& value)
{
    const auto property = std::ranges::find(properties, name, [](const QMetaProperty& known)
                                            { return QString::fromLatin1(known.name()); });
    return property != properties.end() && property->write(&component, value.toVariant());
}

// The one of interfaces named name; nullptr when none is.
InterfaceItem* interfaceNamed(const QList<InterfaceItem*>& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, &InterfaceItem::name);
    return found != interfaces.end() ? *found : nullptr;
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

QJsonObject EditionScene::save() const
{
    QJsonArray components;
    QJsonArray bindings;
    QJsonArray zones;
    // From the first one added: load() adds them in this order, which is then the same again.
    const QList<QGraphicsItem*> added = items(Qt::AscendingOrder);
    for(const QGraphicsItem* item : added)
    {
        if(const auto* componentItem = qobject_cast<const ComponentItem*>(item->toGraphicsObject()))
        {
            const AppForge::Component& component = componentItem->component();
            const std::optional<AppForge::ComponentInfo> info = m_factory.component(component.componentId());
            QJsonObject json;
            json.insert(u"name", component.objectName());
            json.insert(u"type", component.componentId());
            writePosition(json, componentItem->pos());
            json.insert(u"properties", writableProperties(component, info ? info->properties : QList<QMetaProperty>()));
            components.append(json);

            for(const InterfaceItem* consumed : componentItem->consumedInterfaces())
            {
                // One at most: what a component consumes is bound to one interface.
                for(const LinkItem* link : consumed->links())
                {
                    QJsonObject binding;
                    binding.insert(u"consumer", component.objectName());
                    binding.insert(u"consumed", consumed->name());
                    binding.insert(u"provider", link->provided().component().objectName());
                    binding.insert(u"provided", link->provided().name());
                    bindings.append(binding);
                }
            }
        }
        else if(const auto* zone = qgraphicsitem_cast<const ZoneItem*>(item))
        {
            QJsonObject json;
            json.insert(u"title", zone->title());
            json.insert(u"color", zone->color().name());
            writePosition(json, zone->pos());
            json.insert(u"width", zone->boundingRect().width());
            json.insert(u"height", zone->boundingRect().height());
            zones.append(json);
        }
    }

    QJsonObject project;
    project.insert(u"components", components);
    project.insert(u"bindings", bindings);
    project.insert(u"zones", zones);
    return project;
}

QStringList EditionScene::load(const QJsonObject& project)
{
    reset();
    QStringList problems;

    QHash<QString, ComponentItem*> componentItems; // By the name project gives them
    const QJsonArray components = project.value(u"components").toArray();
    for(const QJsonValue& value : components)
    {
        const QJsonObject json = value.toObject();
        const QString name = json.value(u"name").toString();
        const QString componentId = json.value(u"type").toString();
        const std::optional<AppForge::ComponentInfo> info = m_factory.component(componentId);
        ComponentItem* item = info ? addComponent(componentId, readPosition(json), name) : nullptr;
        if(item == nullptr)
        {
            problems.append(tr("%1 is missing: no loaded plugin provides %2").arg(name, componentId));
            continue;
        }
        componentItems.insert(name, item);

        const QJsonObject properties = json.value(u"properties").toObject();
        for(auto property = properties.begin(); property != properties.end(); ++property)
        {
            if(!writeProperty(item->component(), info->properties, property.key(), property.value()))
            {
                problems.append(tr("The property %1 of %2 could not be set").arg(property.key(), name));
            }
        }
    }

    const QJsonArray bindings = project.value(u"bindings").toArray();
    for(const QJsonValue& value : bindings)
    {
        const QJsonObject json = value.toObject();
        const QString consumerName = json.value(u"consumer").toString();
        const QString consumedName = json.value(u"consumed").toString();
        const QString providerName = json.value(u"provider").toString();
        const QString providedName = json.value(u"provided").toString();
        const ComponentItem* consumer = componentItems.value(consumerName);
        const ComponentItem* provider = componentItems.value(providerName);
        InterfaceItem* consumed =
            consumer != nullptr ? interfaceNamed(consumer->consumedInterfaces(), consumedName) : nullptr;
        InterfaceItem* provided =
            provider != nullptr ? interfaceNamed(provider->providedInterfaces(), providedName) : nullptr;
        if(consumed == nullptr || provided == nullptr || !link(*consumed, *provided))
        {
            problems.append(tr("%1 of %2 could not be bound to %3 of %4")
                                .arg(consumedName, consumerName, providedName, providerName));
        }
    }

    const QJsonArray zones = project.value(u"zones").toArray();
    for(const QJsonValue& value : zones)
    {
        const QJsonObject json = value.toObject();
        addZone(readPosition(json), QSizeF(json.value(u"width").toDouble(), json.value(u"height").toDouble()),
                json.value(u"title").toString(), QColor::fromString(json.value(u"color").toString()));
    }
    return problems;
}

void EditionScene::reset()
{
    // Nothing is selected, nor being linked, any longer: who listens lets go of the components before they go.
    clearSelection();
    m_linkSource = nullptr;
    m_pendingLink = nullptr;
    clear(); // Its items first: they refer to the components
    m_assembly.clear();
    m_zoneCount = 0;
    update(HintRect); // Shown again
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
    ComponentItem* item = addComponent(componentId, event->scenePos());
    if(item == nullptr)
    {
        event->ignore();
        return;
    }
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
        addZone(event->scenePos(), ZoneSize, tr("Zone"))->editTitle(); // Named at once
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

ComponentItem* EditionScene::addComponent(const QString& componentId, const QPointF& position, const QString& name)
{
    const std::optional<AppForge::ComponentInfo> info = m_factory.component(componentId);
    AppForge::Component* component = info ? m_assembly.instantiate(componentId, name) : nullptr;
    if(component == nullptr)
    {
        return nullptr;
    }
    auto* item = new ComponentItem(*component, info->name);
    item->setPos(position);
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    return item;
}

bool EditionScene::link(InterfaceItem& source, InterfaceItem& target)
{
    const bool sourceProvided = source.kind() == InterfaceItem::Kind::Provided;
    InterfaceItem& provided = sourceProvided ? source : target;
    InterfaceItem& consumed = sourceProvided ? target : source;
    if(!consumed.component().bindInterface(consumed.name(), provided.component(), provided.name()))
    {
        return false;
    }
    // What a component consumes is bound to one interface: the new one.
    while(!consumed.links().isEmpty())
    {
        delete consumed.links().constFirst();
    }
    addItem(new LinkItem(provided, consumed));
    return true;
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

ZoneItem* EditionScene::addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color)
{
    const QColor next(ZoneColors.at(m_zoneCount++ % ZoneColors.size()));
    auto* zone = new ZoneItem(size, title, color.isValid() ? color : next);
    zone->setPos(position);
    addItem(zone);
    update(HintRect); // Hidden once anything is in the edition zone
    return zone;
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
