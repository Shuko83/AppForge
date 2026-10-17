// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsItem>
#include <QLineF>
#include <QPointer>

class ComponentItem;
class SettingsItem;

// A settings file joined to a component whose properties it holds, in the edition zone: a dashed line between their
// centers, under them, which only shows between them.
class SettingsLinkItem : public QGraphicsItem
{
  public:
    // Deleted with settings, which follow() is called by.
    SettingsLinkItem(SettingsItem& settings, ComponentItem& component);
    ~SettingsLinkItem() override;

    // The component it joins to its settings file; nullptr once it is deleted.
    [[nodiscard]] ComponentItem* component() const;
    // Draws it between its items again, where they are now.
    void follow();

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  private:
    SettingsItem& m_settings;
    QPointer<ComponentItem> m_component;
    QLineF m_line; // In the scene: the item stays at its origin
};
