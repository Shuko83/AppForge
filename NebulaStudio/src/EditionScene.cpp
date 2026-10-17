// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionScene.h"

#include <QColorDialog>
#include <QDir>
#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QHash>
#include <QJsonArray>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QToolTip>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "ComponentItem.h"
#include "ComponentModel.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Settings/SettingsDialog.h"
#include "Settings/SettingsItem.h"
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
// The colors the zones are added with, in turn, which tells them apart until theirs is chosen: those of a nebula,
// violet, blue, pink, cyan, magenta and indigo, which their titles contrast with, in black or white, as WCAG AA asks.
constexpr std::array<QRgb, 6> ZoneColors = {0x7C3AED, 0x3B82F6, 0xEC4899, 0x06B6D4, 0xC026D3, 0x6366F1};

// Accepts a drag carrying a component, as a copy: the component stays in its list. Not while the application is not
// editable.
void acceptComponent(QGraphicsSceneDragDropEvent& event, bool editable)
{
    if(editable && event.mimeData()->hasFormat(ComponentModel::MimeType))
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

// The one of interfaces named name; nullptr when none is.
InterfaceItem* interfaceNamed(const QList<InterfaceItem*>& interfaces, const QString& name)
{
    const auto found = std::ranges::find(interfaces, name, &InterfaceItem::name);
    return found != interfaces.end() ? *found : nullptr;
}

} // namespace

EditionScene::EditionScene(Nebula::Universe& application, const Nebula::ComponentFactory& factory, QObject* parent)
    : QGraphicsScene(SceneRect, parent), m_application(application), m_factory(factory)
{
    connect(&m_application, &Nebula::Universe::runningChanged, this, [this](bool running) { setEditable(!running); });
}

Nebula::Component* EditionScene::selectedComponent() const
{
    // Only the components can be selected.
    const QList<QGraphicsItem*> selected = selectedItems();
    const auto* item =
        selected.size() == 1 ? qobject_cast<const ComponentItem*>(selected.constFirst()->toGraphicsObject()) : nullptr;
    return item != nullptr ? &item->component() : nullptr;
}

QJsonObject EditionScene::save() const
{
    QHash<QString, const ComponentItem*> shown; // The components, by name
    QHash<const Nebula::SettingsFile*, const SettingsItem*> shownSettings;
    QJsonArray zones;
    // From the first one added: load() adds them in this order, which is then the same again.
    const QList<QGraphicsItem*> added = items(Qt::AscendingOrder);
    for(const QGraphicsItem* item : added)
    {
        if(const auto* componentItem = qobject_cast<const ComponentItem*>(item->toGraphicsObject()))
        {
            shown.insert(componentItem->component().objectName(), componentItem);
        }
        else if(const auto* settingsItem = qobject_cast<const SettingsItem*>(item->toGraphicsObject()))
        {
            shownSettings.insert(&settingsItem->file(), settingsItem);
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

    QJsonObject project = m_application.save();
    QJsonArray components = project.value(u"components").toArray();
    for(QJsonValueRef component : components)
    {
        QJsonObject json = component.toObject();
        if(const ComponentItem* item = shown.value(json.value(u"name").toString()))
        {
            writePosition(json, item->pos());
            if(item->color().isValid()) // None while it has the colors of the style
            {
                json.insert(u"color", item->color().name(QColor::HexArgb)); // With its opacity
            }
        }
        component = json;
    }
    project.insert(u"components", components);

    // In the order of the files of the application, as it wrote them.
    QJsonArray settings = project.value(u"settings").toArray();
    const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files = m_application.settingsFiles();
    for(qsizetype index = 0; index < settings.size(); ++index)
    {
        QJsonObject json = settings.at(index).toObject();
        if(const SettingsItem* item = shownSettings.value(files.at(static_cast<std::size_t>(index)).get()))
        {
            writePosition(json, item->pos());
        }
        settings.replace(index, json);
    }
    if(!settings.isEmpty())
    {
        project.insert(u"settings", settings);
    }
    project.insert(u"zones", zones);
    return project;
}

QStringList EditionScene::load(const QJsonObject& project)
{
    removeItems(); // Before the components they show are replaced
    QStringList problems = m_application.load(project);

    QHash<QString, QJsonObject> saved; // What the project holds of the components, by name
    const QJsonArray components = project.value(u"components").toArray();
    for(const QJsonValue& value : components)
    {
        const QJsonObject json = value.toObject();
        saved.insert(json.value(u"name").toString(), json);
    }
    QHash<const Nebula::Component*, ComponentItem*> shown;
    for(const std::unique_ptr<Nebula::Component>& component : m_application.components())
    {
        const QJsonObject json = saved.value(component->objectName());
        ComponentItem* item = showComponent(*component, readPosition(json));
        item->setColor(QColor::fromString(json.value(u"color").toString())); // Not valid when it has none
        shown.insert(component.get(), item);
    }
    // The links of what the application bound, once every component is shown.
    for(const std::unique_ptr<Nebula::Component>& component : m_application.components())
    {
        for(InterfaceItem* consumed : shown.value(component.get())->consumedInterfaces())
        {
            const Nebula::InterfaceBinding bound = component->binding(consumed->name());
            const ComponentItem* provider = shown.value(bound.provider);
            InterfaceItem* provided =
                provider != nullptr ? interfaceNamed(provider->providedInterfaces(), bound.providedName) : nullptr;
            if(provided != nullptr)
            {
                addItem(new LinkItem(*provided, *consumed));
            }
        }
    }
    // Once the components they are joined to are shown: one per file of the project, in its order.
    const QJsonArray settings = project.value(u"settings").toArray();
    const std::vector<std::unique_ptr<Nebula::SettingsFile>>& files = m_application.settingsFiles();
    for(std::size_t index = 0; index < files.size(); ++index)
    {
        showSettings(*files.at(index), readPosition(settings.at(static_cast<qsizetype>(index)).toObject()));
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
    removeItems();
    m_application.clear();
}

void EditionScene::remove(const QList<Nebula::Component*>& components)
{
    // Found before any goes: its interfaces and their links, which are items too, go with it.
    QList<ComponentItem*> removed;
    const QList<QGraphicsItem*> shown = items();
    for(QGraphicsItem* item : shown)
    {
        auto* componentItem = qobject_cast<ComponentItem*>(item->toGraphicsObject());
        if(componentItem != nullptr && components.contains(&componentItem->component()))
        {
            removed.append(componentItem);
        }
    }
    for(ComponentItem* item : std::as_const(removed))
    {
        remove(*item);
    }
}

void EditionScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event, isEditable());
}

void EditionScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event, isEditable());
}

void EditionScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    const QString componentId = QString::fromUtf8(event->mimeData()->data(ComponentModel::MimeType));
    Nebula::Component* component = m_application.instantiate(componentId);
    if(component == nullptr)
    {
        event->ignore();
        return;
    }
    ComponentItem* item = showComponent(*component, event->scenePos());
    clearSelection();
    item->setSelected(true);
    if(event->widget() != nullptr)
    {
        event->widget()->setFocus(); // The view: the keys are for what was just dropped, as Delete is
    }
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void EditionScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    InterfaceItem* source =
        isEditable() && event->button() == Qt::LeftButton ? interfaceAt(event->scenePos()) : nullptr;
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
    if(target != nullptr && isEditable()) // Not when it was started while the link was drawn
    {
        link(*source, *target);
    }
    event->accept();
}

void EditionScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    SettingsItem* settings =
        isEditable() && event->button() == Qt::LeftButton ? settingsAt(event->scenePos()) : nullptr;
    if(settings == nullptr)
    {
        QGraphicsScene::mouseDoubleClickEvent(event);
        return;
    }
    editSettings(*settings, event->widget());
    event->accept();
}

void EditionScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Not while a link is drawn or an item is dragged: what is dragged could be removed from under the cursor.
    if(m_linkSource != nullptr || mouseGrabberItem() != nullptr)
    {
        return;
    }
    if(!isEditable())
    {
        refuseWhileRunning(); // Everything it does edits the application
        event->accept();
        return;
    }
    ComponentItem* component = componentAt(event->scenePos());
    SettingsItem* settings = component == nullptr ? settingsAt(event->scenePos()) : nullptr;
    ZoneItem* zone = zoneAt(event->scenePos());
    QMenu menu(event->widget());
    const QAction* renameComponent = nullptr;
    const QAction* recolorComponent = nullptr;
    QAction* uncolorComponent = nullptr;
    const QAction* removeComponent = nullptr;
    const QAction* editSettingsFile = nullptr;
    QAction* writeDefaultValues = nullptr;
    const QAction* removeSettingsFile = nullptr;
    const QAction* add = nullptr;
    const QAction* addSettingsFile = nullptr;
    if(component != nullptr)
    {
        renameComponent = menu.addAction(tr("Rename component"));
        recolorComponent = menu.addAction(tr("Change component color..."));
        uncolorComponent = menu.addAction(tr("Reset component color"));
        uncolorComponent->setEnabled(component->color().isValid());
        removeComponent = menu.addAction(tr("Remove component"));
    }
    else if(settings != nullptr)
    {
        editSettingsFile = menu.addAction(tr("Edit settings file..."));
        writeDefaultValues = menu.addAction(tr("Write default values"));
        writeDefaultValues->setToolTip(
            tr("Writes the values of the project to the file, in place of those of the user"));
        removeSettingsFile = menu.addAction(tr("Remove settings file"));
    }
    else
    {
        // Not on a component: right-clicking it is about it, and about the zone it is in.
        add = menu.addAction(tr("Add zone"));
        addSettingsFile = menu.addAction(tr("Add settings file"));
    }
    const QAction* renameZone = nullptr;
    const QAction* recolorZone = nullptr;
    const QAction* removeZone = nullptr;
    if(zone != nullptr)
    {
        menu.addSeparator();
        renameZone = menu.addAction(tr("Rename zone"));
        recolorZone = menu.addAction(tr("Change zone color..."));
        removeZone = menu.addAction(tr("Remove zone"));
    }
    // Done once the menu is closed, and the focus back in the view: editing a name takes it.
    const QAction* chosen = menu.exec(event->screenPos());
    event->accept();
    if(chosen == nullptr)
    {
        return;
    }
    if(chosen == renameComponent)
    {
        component->editName();
    }
    else if(chosen == recolorComponent)
    {
        // From the color it has: the one of the style until it is given one.
        const QColor current = component->color().isValid() ? component->color() : palette().color(QPalette::Button);
        const QColor color = QColorDialog::getColor(current, event->widget(), tr("Component color"),
                                                    QColorDialog::ShowAlphaChannel); // Its opacity too
        if(color.isValid())
        {
            component->setColor(color);
        }
    }
    else if(chosen == uncolorComponent)
    {
        component->setColor({});
    }
    else if(chosen == removeComponent)
    {
        remove(*component);
    }
    else if(chosen == editSettingsFile)
    {
        editSettings(*settings, event->widget());
    }
    else if(chosen == writeDefaultValues)
    {
        writeDefaults(*settings);
    }
    else if(chosen == removeSettingsFile)
    {
        remove(*settings);
    }
    else if(chosen == add)
    {
        addZone(event->scenePos(), ZoneSize, tr("Zone"))->editTitle(); // Named at once
    }
    else if(chosen == addSettingsFile)
    {
        // What it holds chosen at once.
        editSettings(*showSettings(m_application.addSettingsFile(), event->scenePos()), event->widget());
    }
    else if(chosen == renameZone)
    {
        zone->editTitle();
    }
    else if(chosen == recolorZone)
    {
        const QColor color = QColorDialog::getColor(zone->color(), event->widget(), tr("Zone color"));
        if(color.isValid())
        {
            zone->setColor(color);
        }
    }
    else if(chosen == removeZone)
    {
        delete zone; // Not what is in it
        update(HintRect);
    }
}

void EditionScene::keyPressEvent(QKeyEvent* event)
{
    // Not while a link is drawn or an item is dragged either: what is dragged would be removed from under the cursor.
    if(event->key() != Qt::Key_Delete || focusItem() != nullptr || m_linkSource != nullptr ||
       mouseGrabberItem() != nullptr)
    {
        QGraphicsScene::keyPressEvent(event);
        return;
    }
    if(!isEditable())
    {
        refuseWhileRunning();
        event->accept();
        return;
    }
    QList<ComponentItem*> removed;
    QList<SettingsItem*> removedSettings;
    const QList<QGraphicsItem*> selected = selectedItems();
    for(QGraphicsItem* item : selected)
    {
        if(auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            removed.append(component);
        }
        else if(auto* settings = qobject_cast<SettingsItem*>(item->toGraphicsObject()))
        {
            removedSettings.append(settings);
        }
    }
    clearSelection(); // Told once, rather than as each of them goes
    // The files first: they would otherwise join themselves again to what is left as each of their components goes.
    for(SettingsItem* settings : std::as_const(removedSettings))
    {
        remove(*settings);
    }
    for(ComponentItem* component : std::as_const(removed))
    {
        remove(*component);
    }
    event->accept();
}

bool EditionScene::isEditable() const
{
    return !m_application.isRunning();
}

void EditionScene::setEditable(bool editable)
{
    for(QGraphicsItem* item : items())
    {
        if(auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            component->setEditable(editable);
        }
        else if(auto* settings = qobject_cast<SettingsItem*>(item->toGraphicsObject()))
        {
            settings->setFlag(QGraphicsItem::ItemIsMovable, editable);
        }
        else if(auto* zone = qgraphicsitem_cast<ZoneItem*>(item))
        {
            zone->setEditable(editable);
        }
    }
}

void EditionScene::refuseWhileRunning()
{
    emit refused(tr("The application is not edited while it runs: stop it first"));
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

ComponentItem* EditionScene::showComponent(Nebula::Component& component, const QPointF& position)
{
    const std::optional<Nebula::ComponentInfo> info = m_factory.component(component.componentId());
    auto* item = new ComponentItem(component, info ? info->name : QString());
    item->setPos(position);
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    connect(item, &ComponentItem::nameEdited, this,
            [this, &component](const QString& name) { rename(component, name); });
    return item;
}

ComponentItem* EditionScene::componentAt(const QPointF& position) const
{
    for(const QGraphicsItem* item : items(position))
    {
        // Through what it is the parent of: the dots of its interfaces go past its edges.
        if(auto* component = qobject_cast<ComponentItem*>(item->topLevelItem()->toGraphicsObject()))
        {
            return component;
        }
    }
    return nullptr;
}

void EditionScene::rename(Nebula::Component& component, const QString& name)
{
    // No name is one given up rather than refused.
    if(!name.isEmpty() && !m_application.rename(component, name))
    {
        emit refused(tr("%1 is not renamed: another component is named %2").arg(component.objectName(), name));
    }
}

void EditionScene::remove(ComponentItem& item)
{
    Nebula::Component& component = item.component();
    item.setSelected(false); // Who listens lets go of the component before it goes
    delete &item;            // With its interfaces, and their links
    m_application.remove(component);
    update(HintRect); // Shown again once nothing is left
}

void EditionScene::removeItems()
{
    // Nothing is selected, nor being linked, any longer: who listens lets go of the components before they go.
    clearSelection();
    m_linkSource = nullptr;
    m_pendingLink = nullptr;
    clear();
    m_zoneCount = 0;
    update(HintRect); // Shown again
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

ZoneItem* EditionScene::addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color)
{
    const QColor next(ZoneColors.at(m_zoneCount++ % ZoneColors.size()));
    auto* zone = new ZoneItem(size, title, color.isValid() ? color : next);
    zone->setPos(position);
    addItem(zone);
    update(HintRect); // Hidden once anything is in the edition zone
    return zone;
}

SettingsItem* EditionScene::showSettings(Nebula::SettingsFile& file, const QPointF& position)
{
    auto* item = new SettingsItem(file);
    item->setPos(position);
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    return item;
}

SettingsItem* EditionScene::settingsAt(const QPointF& position) const
{
    QGraphicsItem* item = itemAt(position, QTransform()); // The topmost one
    return item != nullptr ? qobject_cast<SettingsItem*>(item->toGraphicsObject()) : nullptr;
}

void EditionScene::editSettings(const SettingsItem& item, QWidget* parent)
{
    // Not run here: the event that opens it ends first, while it blocks the window.
    auto* dialog = new SettingsDialog(item.file(), m_application, m_factory, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

void EditionScene::writeDefaults(const SettingsItem& item)
{
    Nebula::SettingsFile& file = item.file();
    if(file.writeDefaults())
    {
        emit told(tr("The default values are written to %1").arg(QDir::toNativeSeparators(file.filePath())));
    }
    else
    {
        emit refused(tr("The default values are not written: %1").arg(file.errorString()));
    }
}

void EditionScene::remove(SettingsItem& item)
{
    Nebula::SettingsFile& file = item.file();
    item.setSelected(false);
    delete &item; // With its links
    m_application.removeSettingsFile(file);
    update(HintRect); // Shown again once nothing is left
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
                             "Right-click to add a zone, which groups components, or a settings file"));
    }
    painter->restore();
}
