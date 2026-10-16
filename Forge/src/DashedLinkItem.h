// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsItem>
#include <QLineF>
#include <QPointer>
#include <functional>

class QGraphicsObject;

// An item of the edition zone joined to a component, which it is about: a dashed line between their centers, under
// them, which only shows between them. A settings file joined to a component whose properties it holds, see
// SettingsItem, or what a composite exposes joined to a component whose properties it exposes, see ExposureItem.
class DashedLinkItem : public QGraphicsItem
{
  public:
    // Deleted with from, which follow() is called by; removed is called as it is deleted.
    DashedLinkItem(QGraphicsObject& from, QGraphicsObject& to, std::function<void(DashedLinkItem&)> removed);
    ~DashedLinkItem() override;

    // Draws it between its items again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    QGraphicsObject& m_from;
    QPointer<QGraphicsObject> m_to;
    std::function<void(DashedLinkItem&)> m_removed;
    QLineF m_line; // In the scene: the item stays at its origin
};
