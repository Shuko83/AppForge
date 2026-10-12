#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QJsonObject>
#include <QStringList>
#include <cstddef>

class Assembly;
class ComponentItem;
class InterfaceItem;
class QGraphicsPathItem;
class ZoneItem;

namespace AppForge
{
class Component;
class ComponentFactory;
} // namespace AppForge

// The zone where the application is assembled: a component dragged from a ComponentModel and dropped here is
// instantiated in the Assembly, and shown where it was dropped. Dragging from the dot of an interface to the dot of
// one it can be bound to, a provided and a consumed one of the same type, binds them and draws a link between them;
// meanwhile, the interfaces it cannot be bound to are dimmed. Its context menu adds a zone, which groups components,
// see ZoneItem, and renames, recolors or removes the one under the cursor.
class EditionScene : public QGraphicsScene
{
    Q_OBJECT

  public:
    EditionScene(Assembly& assembly, const AppForge::ComponentFactory& factory, QObject* parent = nullptr);

    // The component selected, when a single one is; nullptr otherwise. See selectionChanged().
    [[nodiscard]] AppForge::Component* selectedComponent() const;

    // What is in the edition zone, as the "components", "bindings" and "zones" of a project file: see README.md.
    [[nodiscard]] QJsonObject save() const;
    // Replaces what is in the edition zone with what project holds, as save() wrote it; the plugins of its components
    // are loaded already. Returns what could not be restored, a sentence each: none when everything was.
    QStringList load(const QJsonObject& project);
    // Empties the edition zone, and the Assembly of its components.
    void reset();

  protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // Pressing the dot of an interface starts a link, which follows the cursor until the button is released.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    // A grid, and a hint while nothing is in the edition zone.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    // The interface whose dot is at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* interfaceAt(const QPointF& position) const;
    // The interface the link being drawn would be bound to if released at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* linkTargetAt(const QPointF& position) const;
    // Dims the interfaces a link drawn from source cannot be bound to; none when source is nullptr.
    void dimLinkTargets(const InterfaceItem* source);
    // Instantiates componentId in the Assembly, named name if it can be, see Assembly::instantiate(), and shows it at
    // position; nullptr when it is not registered.
    ComponentItem* addComponent(const QString& componentId, const QPointF& position, const QString& name = {});
    // Binds the consumed one to the provided one, whichever is which, and draws their link, in place of the one the
    // consumed one had; returns false, doing nothing, when they cannot be bound.
    bool link(InterfaceItem& source, InterfaceItem& target);
    // The zone position is in, the smallest one when several are; nullptr when there is none.
    [[nodiscard]] ZoneItem* zoneAt(const QPointF& position) const;
    // Adds a zone, its top left corner at position; of the next color when color is not valid.
    ZoneItem* addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color = {});

    Assembly& m_assembly;
    const AppForge::ComponentFactory& m_factory;
    InterfaceItem* m_linkSource = nullptr;      // Where the link being drawn starts; nullptr when none is
    QGraphicsPathItem* m_pendingLink = nullptr; // The link being drawn
    std::size_t m_zoneCount = 0;                // The zones added so far: each takes the next color
};
