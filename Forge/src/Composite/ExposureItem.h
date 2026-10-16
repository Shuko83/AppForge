// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class DashedLinkItem;
class InterfaceItem;

namespace AppForge
{
class Composite;
} // namespace AppForge

// What a composite exposes, in its scene: the face it shows to the scene it is placed in, a single item, under the
// name of its file above "Exposed", with a dashed outline. Under them, a row per gateway, its dot on an edge, named as
// the gateway is: on the left those that consume an interface of its components, which the composite provides, on the
// right those that provide one to its components, which the composite consumes; they are linked as the dots of a
// component are. Then a row per property it exposes, its name, and the component it is of. Dashed lines join it to
// the components whose properties it exposes.
// It can be moved and selected, but not removed; double-clicking it asks for what the composite exposes to be chosen,
// see editRequested() and ExposureDialog.
class ExposureItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // composite, one of the application, outlives the item.
    explicit ExposureItem(AppForge::Composite& composite);
    // With its links.
    ~ExposureItem() override;

    [[nodiscard]] AppForge::Composite& composite() const;
    // The dots of its gateways, each in the order of the components of the composite.
    [[nodiscard]] const QList<InterfaceItem*>& consumedInterfaces() const;
    [[nodiscard]] const QList<InterfaceItem*>& providedInterfaces() const;

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  signals:
    // It was double-clicked: what the composite exposes is to be chosen.
    void editRequested();
    // The dots of its gateways were shown again, without their links, as gateways were added, renamed or removed.
    void interfacesRebuilt();

  protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    // Joined to the components of the scene it is added to.
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    // Shows the gateways and the properties of the composite as they are now.
    void refresh();
    // Joins it to the components of its scene whose properties the composite exposes.
    void refreshLinks();
    // Draws its links again, where it and their components are now.
    void followLinks();

    AppForge::Composite& m_composite;
    // Its children, a dot per gateway.
    QList<InterfaceItem*> m_consumedInterfaces;
    QList<InterfaceItem*> m_providedInterfaces;
    QList<QPair<QString, QString>> m_properties; // A row per property exposed: its name, and where it is
    QList<DashedLinkItem*> m_links;              // In its scene rather than its children: under the components
};
