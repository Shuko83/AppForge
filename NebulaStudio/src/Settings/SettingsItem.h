// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsObject>
#include <QList>

class SettingsLinkItem;

namespace Nebula
{
class SettingsFile;
}

// A settings file of the application in the edition zone: the name of its file above "Settings file", on a sheet whose
// corner is folded. Dashed lines join it to the components whose properties it holds, see SettingsLinkItem; its
// tooltip tells where the file is and which properties it holds. It can be moved and selected; EditionScene edits its
// file when it is double-clicked, see SettingsDialog.
class SettingsItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // file, owned by the application, outlives the item.
    explicit SettingsItem(Nebula::SettingsFile& file);
    // With its links.
    ~SettingsItem() override;

    [[nodiscard]] Nebula::SettingsFile& file() const;
    // Called by link as it is deleted.
    void removeLink(SettingsLinkItem& link);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    // Joined to the components of the scene it is added to.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    // Joins it to the components of its scene whose properties its file holds now.
    void refreshLinks();
    // Draws its links again, where it and their components are now.
    void followLinks();
    // Tells where its file is and which properties it holds.
    void refreshToolTip();

    Nebula::SettingsFile& m_file;
    QList<SettingsLinkItem*> m_links; // In its scene rather than its children: under the components
};
