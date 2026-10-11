#pragma once

#include <QGraphicsScene>

class Assembly;
class InterfaceItem;
class QGraphicsPathItem;

namespace AppForge
{
class ComponentFactory;
}

// The zone where the application is assembled: a component dragged from a ComponentModel and dropped here is
// instantiated in the Assembly, and shown where it was dropped. Dragging from the dot of an interface to the dot of
// one it can be bound to, a provided and a consumed one of the same type, binds them and draws a link between them.
class EditionScene : public QGraphicsScene
{
    Q_OBJECT

  public:
    EditionScene(Assembly& assembly, const AppForge::ComponentFactory& factory, QObject* parent = nullptr);

  protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // Pressing the dot of an interface starts a link, which follows the cursor until the button is released.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    // A grid, and a hint while the zone is empty.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    // The interface whose dot is at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* interfaceAt(const QPointF& position) const;
    // The interface the link being drawn would be bound to if released at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* linkTargetAt(const QPointF& position) const;
    // Binds the consumed one to the provided one, whichever is which, and draws their link, in place of the one the
    // consumed one had; does nothing when they cannot be bound.
    void link(InterfaceItem& source, InterfaceItem& target);

    Assembly& m_assembly;
    const AppForge::ComponentFactory& m_factory;
    InterfaceItem* m_linkSource = nullptr;      // Where the link being drawn starts; nullptr when none is
    QGraphicsPathItem* m_pendingLink = nullptr; // The link being drawn
};
