// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsItem>
#include <QPainterPath>

class InterfaceItem;

// An interface a component consumes bound to the one another component provides, in the edition zone: a curve between
// their dots, which follows them.
class LinkItem : public QGraphicsItem
{
  public:
    // Not a child of them, but deleted with the first one deleted.
    LinkItem(InterfaceItem& provided, InterfaceItem& consumed);
    ~LinkItem() override;

    // The curve of a link from a provided interface to a consumed one, their dots being there in the scene.
    [[nodiscard]] static QPainterPath curve(const QPointF& provided, const QPointF& consumed);

    // Draws it between its interfaces again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    InterfaceItem& m_provided;
    InterfaceItem& m_consumed;
    QPainterPath m_curve; // In the scene: the item stays at its origin
};
