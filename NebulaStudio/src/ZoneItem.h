// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QList>
#include <QString>

class NameEditor;

// A colored zone of the edition zone, under a title: it groups the components placed in it, by what they are for, say.
// Dragged by its title, it moves with what is in it then: the components and the settings files whose center is, and
// the zones that entirely are. Dragging an edge, or a corner, resizes it, and double-clicking its title edits it in
// place. Only its title and its edges take the mouse: what is between them is reached as if the zone were not there.
class ZoneItem : public QGraphicsItem
{
  public:
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 2
    };

    // Its position is its top left corner.
    ZoneItem(const QSizeF& size, QString title, const QColor& color);

    [[nodiscard]] int type() const override;
    // As it was before it started being edited, while it is.
    [[nodiscard]] const QString& title() const;
    [[nodiscard]] const QColor& color() const;
    void setColor(const QColor& color);
    // Edits its title in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. It
    // must be in a scene.
    void editTitle();
    // Whether the mouse moves it, resizes it and edits its title: it does until told otherwise. When it does not, it
    // takes no click, which reaches what is under it.
    void setEditable(bool editable);

    [[nodiscard]] QRectF boundingRect() const override;
    // Its title and its edges only.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    // Over an edge or a corner, the cursor tells which way dragging it resizes the zone.
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

  private:
    // Where its title is written.
    [[nodiscard]] QRectF titleRect() const;
    // The edges dragging from position resizes it by: both of a corner near it, none between its edges.
    [[nodiscard]] Qt::Edges edgesAt(const QPointF& position) const;
    // What is in it: the components and the settings files whose center is, and the zones that entirely are.
    [[nodiscard]] QList<QGraphicsItem*> content() const;
    // Moves the edges being dragged to scenePosition, as far as its minimum size lets them.
    void resize(const QPointF& scenePosition);
    // Under the links, and under the zones smaller than it, which can be in it.
    void stack();

    QSizeF m_size;
    QString m_title;
    QColor m_color;
    NameEditor* m_titleEditor = nullptr; // Its child, over its title while it is edited; created when first so
    bool m_moving = false;               // Whether it is being dragged by its title
    QList<QGraphicsItem*> m_followers;   // What was in it when that started, and moves with it
    Qt::Edges m_resizedEdges;            // The edges being dragged; none when it is not being resized
    QPointF m_grabOffset;                // From those edges to where they were grabbed, in the scene
};
