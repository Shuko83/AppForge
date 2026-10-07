#pragma once

#include <QGraphicsScene>

class Assembly;

namespace AppForge
{
class ComponentFactory;
}

// The zone where the application is assembled: a component dragged from a ComponentModel and dropped here is
// instantiated in the Assembly, and shown where it was dropped.
class EditionScene : public QGraphicsScene
{
    Q_OBJECT

  public:
    EditionScene(Assembly& assembly, const AppForge::ComponentFactory& factory, QObject* parent = nullptr);

  protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // A grid, and a hint while the zone is empty.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    Assembly& m_assembly;
    const AppForge::ComponentFactory& m_factory;
};
