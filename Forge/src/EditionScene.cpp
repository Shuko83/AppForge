#include "EditionScene.h"

#include <QGraphicsSceneDragDropEvent>
#include <QList>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <cmath>
#include <optional>

#include "Assembly.h"
#include "Component/ComponentFactory.h"
#include "ComponentItem.h"
#include "ComponentModel.h"

namespace
{

constexpr qreal GridStep = 20;
// Large enough not to grow, so that the view does not move when a component is dropped.
constexpr QRectF SceneRect(0, 0, 4000, 3000);
// Where the hint is drawn while the zone is empty, in its top left corner.
constexpr QRectF HintRect(GridStep, GridStep, 600, 2 * GridStep);

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
    update(HintRect); // Hidden once a component is in the zone
    clearSelection();
    item->setSelected(true);
    event->setDropAction(Qt::CopyAction);
    event->accept();
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
                          tr("Drag a component from the list of components and drop it here to instantiate it"));
    }
    painter->restore();
}
