// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "EditionScene.h"

#include <QColorDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QSaveFile>
#include <QToolTip>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "AppForge/Application/ForgeApplication.h"
#include "AppForge/Component/ComponentFactory.h"
#include "AppForge/Composite/Composite.h"
#include "AppForge/Composite/Gateway.h"
#include "ComponentItem.h"
#include "ComponentModel.h"
#include "Composite/ExposureDialog.h"
#include "Composite/ExposureItem.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
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
// The colors the zones are added with, in turn, which tells them apart until theirs is chosen.
constexpr std::array<QRgb, 6> ZoneColors = {0x4E79A7, 0xF28E2B, 0x59A14F, 0xE15759, 0xB07AA1, 0x76B7B2};

// Accepts a drag carrying a component, or a composite, as a copy: it stays in its list.
void acceptComponent(QGraphicsSceneDragDropEvent& event)
{
    if(event.mimeData()->hasFormat(ComponentModel::MimeType) ||
       event.mimeData()->hasFormat(ComponentModel::CompositeMimeType))
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

} // namespace

EditionScene::EditionScene(AppForge::ForgeApplication& application, const AppForge::ComponentFactory& factory,
                           AppForge::Composite* composite, QObject* parent)
    : QGraphicsScene(SceneRect, parent), m_application(application), m_factory(factory), m_composite(composite)
{
}

AppForge::Composite* EditionScene::composite() const
{
    return m_composite;
}

void EditionScene::setDirectory(const QString& directory)
{
    m_directory = directory;
}

const std::vector<std::unique_ptr<AppForge::Component>>& EditionScene::components() const
{
    return m_composite != nullptr ? m_composite->components() : m_application.components();
}

bool EditionScene::canChange()
{
    if(m_composite != nullptr && !m_application.isComplete(m_composite->filePath()))
    {
        emit refused(tr("%1 could not be entirely read: it is not changed, which would lose what is missing")
                         .arg(QFileInfo(m_composite->filePath()).fileName()));
        return false;
    }
    return true;
}

AppForge::Component* EditionScene::selectedComponent() const
{
    // Only the components can be selected.
    const QList<QGraphicsItem*> selected = selectedItems();
    const auto* item =
        selected.size() == 1 ? qobject_cast<const ComponentItem*>(selected.constFirst()->toGraphicsObject()) : nullptr;
    return item != nullptr ? &item->component() : nullptr;
}

QJsonObject EditionScene::save(const QString& directory) const
{
    QHash<QString, const ComponentItem*> shown; // The components, by name
    QHash<const AppForge::SettingsFile*, const SettingsItem*> shownSettings;
    const ExposureItem* exposure = nullptr;
    QJsonArray zones;
    // From the first one added: load() adds them in this order, which is then the same again.
    const QList<QGraphicsItem*> added = items(Qt::AscendingOrder);
    for(const QGraphicsItem* item : added)
    {
        if(const auto* componentItem = qobject_cast<const ComponentItem*>(item->toGraphicsObject()))
        {
            shown.insert(componentItem->component().objectName(), componentItem);
        }
        else if(const auto* exposureItem = qobject_cast<const ExposureItem*>(item->toGraphicsObject()))
        {
            exposure = exposureItem;
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

    QJsonObject project =
        m_composite != nullptr ? m_application.saveComposite(m_composite->filePath()) : m_application.save(directory);
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
    const std::vector<std::unique_ptr<AppForge::SettingsFile>>& files = m_application.settingsFiles();
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
    if(exposure != nullptr)
    {
        QJsonObject json;
        writePosition(json, exposure->pos());
        project.insert(u"exposure", json);
    }
    project.insert(u"zones", zones);
    return project;
}

QStringList EditionScene::load(const QJsonObject& project, const QString& directory)
{
    removeItems(); // Before the components they show are replaced
    QStringList problems = m_application.load(project, directory);
    showDocument(project);
    return problems;
}

void EditionScene::show(const QJsonObject& document)
{
    removeItems();
    showDocument(document);
}

void EditionScene::showDocument(const QJsonObject& document)
{
    QHash<QString, QJsonObject> saved; // What the document holds of the components, by name
    const QJsonArray components = document.value(u"components").toArray();
    for(const QJsonValue& value : components)
    {
        const QJsonObject json = value.toObject();
        saved.insert(json.value(u"name").toString(), json);
    }
    QRectF bounds; // Of the components shown
    for(const std::unique_ptr<AppForge::Component>& component : this->components())
    {
        // Those of a gateway are on the item of what its composite exposes.
        if(qobject_cast<AppForge::Gateway*>(component.get()) != nullptr)
        {
            continue;
        }
        const QJsonObject json = saved.value(component->objectName());
        ComponentItem* item = showComponent(*component, readPosition(json));
        item->setColor(QColor::fromString(json.value(u"color").toString())); // Not valid when it has none
        bounds |= item->sceneBoundingRect();
    }
    if(m_composite != nullptr)
    {
        // Where its file put it; otherwise next to the components, on the side its links leave from: on the right of
        // them when it mostly takes what they provide, on their left when it mostly gives them what they consume.
        qsizetype giving = 0;
        for(const std::unique_ptr<AppForge::Component>& component : m_composite->components())
        {
            const auto* gateway = qobject_cast<const AppForge::Gateway*>(component.get());
            giving += gateway == nullptr ? 0 : gateway->kind() == AppForge::GatewayKind::Consumed ? 1 : -1;
        }
        const QJsonObject json = document.value(u"exposure").toObject();
        QPointF position = readPosition(json);
        if(json.isEmpty())
        {
            const qreal x =
                giving > 0 ? std::max(bounds.left() - 14 * GridStep, 6 * GridStep) : bounds.right() + 10 * GridStep;
            position = bounds.isNull() ? QPointF(12 * GridStep, 6 * GridStep) : QPointF(x, bounds.top() + 2 * GridStep);
        }
        auto* exposure = new ExposureItem(*m_composite);
        exposure->setPos(position);
        addItem(exposure);
        connect(exposure, &ExposureItem::interfacesRebuilt, this, &EditionScene::showLinks);
        connect(exposure, &ExposureItem::editRequested, this,
                [this] { editExposure(views().isEmpty() ? nullptr : views().constFirst()); });
    }
    // The links of what the application bound, once every component is shown.
    showLinks();
    // Once the components they are joined to are shown: one per file of the project, in its order. Those of the
    // application, in its scene.
    if(m_composite == nullptr)
    {
        const QJsonArray settings = document.value(u"settings").toArray();
        const std::vector<std::unique_ptr<AppForge::SettingsFile>>& files = m_application.settingsFiles();
        for(std::size_t index = 0; index < files.size(); ++index)
        {
            showSettings(*files.at(index), readPosition(settings.at(static_cast<qsizetype>(index)).toObject()));
        }
    }

    const QJsonArray zones = document.value(u"zones").toArray();
    for(const QJsonValue& value : zones)
    {
        const QJsonObject json = value.toObject();
        addZone(readPosition(json), QSizeF(json.value(u"width").toDouble(), json.value(u"height").toDouble()),
                json.value(u"title").toString(), QColor::fromString(json.value(u"color").toString()));
    }
}

void EditionScene::reset()
{
    removeItems();
    m_application.clear();
}

void EditionScene::remove(const QList<AppForge::Component*>& components)
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
    acceptComponent(*event);
}

void EditionScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    acceptComponent(*event);
}

void EditionScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    if(!canChange())
    {
        event->ignore();
        return;
    }
    AppForge::Component* component = nullptr;
    if(event->mimeData()->hasFormat(ComponentModel::CompositeMimeType))
    {
        const QString file = QString::fromUtf8(event->mimeData()->data(ComponentModel::CompositeMimeType));
        component = m_application.instantiateComposite(file, {}, m_composite);
        // Placed or not, it can have been read now, and the composites it holds too.
        emit compositesChanged();
        if(component == nullptr || !m_application.errorString().isEmpty())
        {
            emit refused(component == nullptr
                             ? tr("%1 is not placed: %2").arg(QFileInfo(file).fileName(), m_application.errorString())
                             : tr("%1 could not be entirely read: %2")
                                   .arg(QFileInfo(file).fileName(), m_application.errorString().section(u'\n', 0, 0)));
        }
    }
    else
    {
        component = m_application.instantiate(QString::fromUtf8(event->mimeData()->data(ComponentModel::MimeType)), {},
                                              m_composite);
    }
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

void EditionScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    SettingsItem* settings = event->button() == Qt::LeftButton ? settingsAt(event->scenePos()) : nullptr;
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
    // What its composite exposes: chosen in a dialog.
    QGraphicsItem* under = itemAt(event->scenePos(), QTransform());
    if(under != nullptr && qobject_cast<ExposureItem*>(under->topLevelItem()->toGraphicsObject()) != nullptr)
    {
        QMenu exposureMenu(event->widget());
        const QAction* choose = exposureMenu.addAction(tr("Choose what is exposed..."));
        event->accept();
        if(exposureMenu.exec(event->screenPos()) == choose)
        {
            editExposure(event->widget());
        }
        return;
    }
    ComponentItem* component = componentAt(event->scenePos());
    // Those of the application only.
    SettingsItem* settings = component == nullptr && m_composite == nullptr ? settingsAt(event->scenePos()) : nullptr;
    ZoneItem* zone = zoneAt(event->scenePos());
    QMenu menu(event->widget());
    const QAction* openComposite = nullptr;
    QAction* startComponent = nullptr;
    const QAction* renameComponent = nullptr;
    const QAction* recolorComponent = nullptr;
    QAction* uncolorComponent = nullptr;
    const QAction* removeComponent = nullptr;
    const QAction* group = nullptr;
    const QAction* editSettingsFile = nullptr;
    QAction* writeDefaultValues = nullptr;
    const QAction* removeSettingsFile = nullptr;
    const QAction* add = nullptr;
    const QAction* addSettingsFile = nullptr;
    if(component != nullptr)
    {
        if(qobject_cast<AppForge::Composite*>(&component->component()) != nullptr)
        {
            openComposite = menu.addAction(tr("Open composite"));
            menu.addSeparator();
        }
        // Only while the application runs, and the composite it is of: one dropped meanwhile; otherwise, Start starts
        // them all.
        startComponent = menu.addAction(tr("Start component"));
        startComponent->setEnabled(
            m_application.isRunning() && component->component().state() != AppForge::ComponentState::Running &&
            (m_composite == nullptr || m_composite->state() == AppForge::ComponentState::Running));
        menu.addSeparator();
        renameComponent = menu.addAction(tr("Rename component"));
        recolorComponent = menu.addAction(tr("Change component color..."));
        uncolorComponent = menu.addAction(tr("Reset component color"));
        uncolorComponent->setEnabled(component->color().isValid());
        removeComponent = menu.addAction(tr("Remove component"));
        // The selection, which the component is part of: in the scene of the application, whose components they are.
        if(m_composite == nullptr && component->isSelected())
        {
            menu.addSeparator();
            group = menu.addAction(tr("Group into composite..."));
        }
    }
    else if(settings != nullptr)
    {
        editSettingsFile = menu.addAction(tr("Edit settings file..."));
        writeDefaultValues = menu.addAction(tr("Write default values"));
        writeDefaultValues->setToolTip(
            tr("Writes the values of the project to the file, in place of those of the user"));
        writeDefaultValues->setEnabled(!m_application.isRunning()); // The properties have the values of the user
        removeSettingsFile = menu.addAction(tr("Remove settings file"));
    }
    else
    {
        // Not on a component: right-clicking it is about it, and about the zone it is in.
        add = menu.addAction(tr("Add zone"));
        if(m_composite == nullptr)
        {
            addSettingsFile = menu.addAction(tr("Add settings file"));
        }
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
    if(chosen == openComposite)
    {
        emit openRequested(*qobject_cast<AppForge::Composite*>(&component->component()));
    }
    else if(chosen == group)
    {
        groupSelection(event->widget());
    }
    else if(chosen == startComponent)
    {
        if(!m_application.start(component->component()))
        {
            QMessageBox::warning(event->widget(), tr("Start"),
                                 tr("The component cannot be started:\n%1").arg(m_application.errorString()));
        }
    }
    else if(chosen == renameComponent)
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
        if(canChange())
        {
            remove(*component);
        }
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
    event->accept();
    if(!canChange())
    {
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

ComponentItem* EditionScene::showComponent(AppForge::Component& component, const QPointF& position)
{
    // What it is an instance of: its component, or its composite file.
    auto* composite = qobject_cast<AppForge::Composite*>(&component);
    const std::optional<AppForge::ComponentInfo> info = m_factory.component(component.componentId());
    auto* item = new ComponentItem(component, composite != nullptr ? QFileInfo(composite->filePath()).completeBaseName()
                                              : info               ? info->name
                                                                   : QString());
    item->setPos(position);
    addItem(item);
    update(HintRect); // Hidden once anything is in the edition zone
    connect(item, &ComponentItem::nameEdited, this,
            [this, &component](const QString& name) { rename(component, name); });
    if(composite != nullptr)
    {
        connect(item, &ComponentItem::openRequested, this, [this, composite] { emit openRequested(*composite); });
    }
    connect(item, &ComponentItem::interfacesRebuilt, this, &EditionScene::showLinks);
    return item;
}

void EditionScene::showLinks()
{
    // The dots of the scene: those of its components, and of what its composite exposes.
    QList<InterfaceItem*> dots;
    const QList<QGraphicsItem*> shown = items();
    for(QGraphicsItem* item : shown)
    {
        if(auto* dot = qgraphicsitem_cast<InterfaceItem*>(item))
        {
            dots.append(dot);
        }
    }
    for(InterfaceItem* consumed : std::as_const(dots))
    {
        if(consumed->kind() != InterfaceItem::Kind::Consumed)
        {
            continue;
        }
        const AppForge::InterfaceBinding bound = consumed->component().binding(consumed->name());
        // Not those of what it is no longer bound to: it was unbound, or bound to another meanwhile.
        const QList<LinkItem*> links = consumed->links();
        for(LinkItem* link : links)
        {
            if(&link->provided().component() != bound.provider || link->provided().name() != bound.providedName)
            {
                delete link;
            }
        }
        // A consumed interface is bound to one interface at most.
        if(!consumed->links().isEmpty())
        {
            continue;
        }
        const auto provided = std::ranges::find_if(dots,
                                                   [&bound](const InterfaceItem* dot)
                                                   {
                                                       return dot->kind() == InterfaceItem::Kind::Provided &&
                                                              &dot->component() == bound.provider &&
                                                              dot->name() == bound.providedName;
                                                   });
        if(bound.provider != nullptr && provided != dots.end())
        {
            addItem(new LinkItem(**provided, *consumed));
        }
    }
}

void EditionScene::editExposure(QWidget* parent)
{
    // Not run here: the event that opens it ends first, while it blocks the window.
    auto* dialog = new ExposureDialog(*m_composite, m_application, m_factory, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // Once what was chosen is given to the composite: its gateways are bound, or unbound.
    connect(dialog, &QDialog::accepted, this, &EditionScene::showLinks);
    dialog->open();
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

void EditionScene::rename(AppForge::Component& component, const QString& name)
{
    // No name is one given up rather than refused.
    if(name.isEmpty() || name == component.objectName() || !canChange())
    {
        return;
    }
    if(name.contains(u'/'))
    {
        emit refused(tr("%1 is not renamed: a name holds no /, which separates the names of a path")
                         .arg(component.objectName()));
    }
    else if(!m_application.rename(component, name))
    {
        emit refused(tr("%1 is not renamed: another component is named %2").arg(component.objectName(), name));
    }
}

void EditionScene::remove(ComponentItem& item)
{
    AppForge::Component& component = item.component();
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
    if(!canChange())
    {
        return;
    }
    // Through the application: in a composite, every instance of its file is bound alike.
    if(!m_application.bind(consumed.component(), consumed.name(), provided.component(), provided.name()))
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

void EditionScene::groupSelection(QWidget* parent)
{
    QList<ComponentItem*> items;
    QList<AppForge::Component*> components;
    QRectF bounds;
    const QList<QGraphicsItem*> selected = selectedItems();
    for(QGraphicsItem* item : selected)
    {
        if(auto* component = qobject_cast<ComponentItem*>(item->toGraphicsObject()))
        {
            items.append(component);
            components.append(&component->component());
            bounds |= component->sceneBoundingRect();
        }
    }
    if(items.isEmpty())
    {
        return;
    }
    const QString filePath = QFileDialog::getSaveFileName(parent, tr("Group into composite"), m_directory,
                                                          tr("AppForge composites (*.afc)"));
    if(filePath.isEmpty())
    {
        return;
    }
    // Its instances have the structure they read from it: it is not written over.
    const QString canonical = QFileInfo(filePath).canonicalFilePath();
    if(!canonical.isEmpty() && m_application.compositeFiles().contains(canonical))
    {
        QMessageBox::warning(
            parent, tr("Group into composite"),
            tr("%1 is a composite of the application: choose another file").arg(QDir::toNativeSeparators(filePath)));
        return;
    }
    std::optional<QJsonObject> document = m_application.compose(components, QFileInfo(filePath).absolutePath());
    if(!document)
    {
        QMessageBox::warning(parent, tr("Group into composite"),
                             tr("These components cannot be grouped:\n%1").arg(m_application.errorString()));
        return;
    }

    // Where they are from one another, in the top left corner of the scene of the composite.
    const QPointF origin = bounds.topLeft() - QPointF(4 * GridStep, 4 * GridStep);
    QJsonArray entries = document->value(u"components").toArray();
    for(QJsonValueRef entry : entries)
    {
        QJsonObject json = entry.toObject();
        const auto item = std::ranges::find(items, json.value(u"name").toString(),
                                            [](const ComponentItem* shown) { return shown->component().objectName(); });
        if(item != items.end())
        {
            writePosition(json, (*item)->pos() - origin);
            if((*item)->color().isValid())
            {
                json.insert(u"color", (*item)->color().name(QColor::HexArgb));
            }
        }
        entry = json;
    }
    document->insert(u"components", entries);
    document->insert(u"zones", QJsonArray());

    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(*document).toJson()) < 0 || !file.commit())
    {
        QMessageBox::warning(
            parent, tr("Group into composite"),
            tr("%1 cannot be written:\n%2").arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return;
    }
    // Read before their items go: they stay when it cannot be.
    if(!m_application.loadComposite(filePath))
    {
        QMessageBox::warning(
            parent, tr("Group into composite"),
            tr("%1 cannot be placed:\n%2").arg(QDir::toNativeSeparators(filePath), m_application.errorString()));
        emit compositesChanged();
        return;
    }
    clearSelection();
    for(ComponentItem* item : std::as_const(items))
    {
        delete item; // With its links, before its component goes
    }
    AppForge::Composite* composite = m_application.group(components, filePath);
    emit compositesChanged();
    if(composite != nullptr)
    {
        // Where they were, as its scene shows them.
        ComponentItem* item = showComponent(*composite, bounds.center());
        showLinks(); // To what its gateways bound it to
        item->setSelected(true);
        emit told(tr("%1 holds them now").arg(QDir::toNativeSeparators(composite->filePath())));
    }
    update(HintRect);
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

SettingsItem* EditionScene::showSettings(AppForge::SettingsFile& file, const QPointF& position)
{
    auto* item = new SettingsItem(m_application, file);
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
    AppForge::SettingsFile& file = item.file();
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
    AppForge::SettingsFile& file = item.file();
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
                          m_composite != nullptr
                              ? tr("Drag a component from the list of components and drop it here to add it to %1\n"
                                   "Right-click to add a zone, which groups components")
                                    .arg(QFileInfo(m_composite->filePath()).fileName())
                              : tr("Drag a component from the list of components and drop it here to instantiate it\n"
                                   "Right-click to add a zone, which groups components, or a settings file"));
    }
    painter->restore();
}
