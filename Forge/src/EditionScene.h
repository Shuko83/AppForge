// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QStringList>
#include <cstddef>

class ComponentItem;
class InterfaceItem;
class QGraphicsPathItem;
class ZoneItem;

namespace AppForge
{
class Component;
class ComponentFactory;
class ForgeApplication;
} // namespace AppForge

// The zone where the application is assembled: a component dragged from a ComponentModel and dropped here is
// instantiated in the application, and shown where it was dropped. Dragging from the dot of an interface to the dot of
// one it can be bound to, a provided and a consumed one of the same type, binds them and draws a link between them;
// meanwhile, the interfaces it cannot be bound to are dimmed. Its context menu adds a zone, which groups components,
// see ZoneItem, unless it is opened on a component, and renames, recolors or removes the one under the cursor; it
// renames and recolors the component under the cursor too, see ComponentItem. A component keeps a name no other one of
// the application has: see refused().
// The same menu removes the component under the cursor from the application, with its links, and the Delete key
// removes the selected ones. It also starts that component, unless it runs, which runs the application.
class EditionScene : public QGraphicsScene
{
    Q_OBJECT

  public:
    EditionScene(AppForge::ForgeApplication& application, const AppForge::ComponentFactory& factory,
                 QObject* parent = nullptr);

    // The component selected, when a single one is; nullptr otherwise. See selectionChanged().
    [[nodiscard]] AppForge::Component* selectedComponent() const;

    // The application as a project file holds it, see README.md: what ForgeApplication::save() writes, with where its
    // components are in the edition zone and the color they were given, and its zones.
    [[nodiscard]] QJsonObject save() const;
    // Replaces the application with the one project holds, as save() wrote it, see ForgeApplication::load(), and shows
    // it as it was. Returns what could not be restored, a sentence each: none when everything was.
    QStringList load(const QJsonObject& project);
    // Empties the edition zone, and the application of its components.
    void reset();
    // Removes components, some of the application, from it and from the edition zone, with their links.
    void remove(const QList<AppForge::Component*>& components);

  signals:
    // What was just asked of it is not done; why is a sentence to show.
    void refused(const QString& why);

  protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // Pressing the dot of an interface starts a link, which follows the cursor until the button is released.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    // Delete removes the selected components, unless a name is edited: the key is for what is typed then.
    void keyPressEvent(QKeyEvent* event) override;
    // A grid, and a hint while nothing is in the edition zone.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    // The interface whose dot is at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* interfaceAt(const QPointF& position) const;
    // The interface the link being drawn would be bound to if released at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* linkTargetAt(const QPointF& position) const;
    // Dims the interfaces a link drawn from source cannot be bound to; none when source is nullptr.
    void dimLinkTargets(const InterfaceItem* source);
    // Shows component, one of the application, at position.
    ComponentItem* showComponent(AppForge::Component& component, const QPointF& position);
    // The component shown at position, or whose interface is; nullptr when there is none.
    [[nodiscard]] ComponentItem* componentAt(const QPointF& position) const;
    // Names component name, as typed in place of its name; tells why through refused() when it cannot be.
    void rename(AppForge::Component& component, const QString& name);
    // Removes item, with the links of its interfaces, and its component from the application.
    void remove(ComponentItem& item);
    // Empties the edition zone, while the components it shows still exist.
    void removeItems();
    // Binds the consumed one to the provided one, whichever is which, and draws their link, in place of the one the
    // consumed one had; does nothing when they cannot be bound.
    void link(InterfaceItem& source, InterfaceItem& target);
    // The zone position is in, the smallest one when several are; nullptr when there is none.
    [[nodiscard]] ZoneItem* zoneAt(const QPointF& position) const;
    // Adds a zone, its top left corner at position; of the next color when color is not valid.
    ZoneItem* addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color = {});

    AppForge::ForgeApplication& m_application;
    const AppForge::ComponentFactory& m_factory;
    InterfaceItem* m_linkSource = nullptr;      // Where the link being drawn starts; nullptr when none is
    QGraphicsPathItem* m_pendingLink = nullptr; // The link being drawn
    std::size_t m_zoneCount = 0;                // The zones added so far: each takes the next color
};
