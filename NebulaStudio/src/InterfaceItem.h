// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QList>
#include <QMetaType>
#include <QString>
#include <cstdint>

class ComponentItem;
class LinkItem;

namespace Nebula
{
class Component;
struct InterfaceInfo;
} // namespace Nebula

// An interface of a component, on an edge of its ComponentItem: a dot, with its name next to it, inside the component.
// One the component provides is on its right edge: a disc while it is available, a dotted circle otherwise. One it
// consumes is on its left edge: a circle, with an asterisk after its name when the component needs it, both in the
// warning color while it is bound to nothing. Hovering the dot tells, in its tooltip, whether the interface is provided
// or consumed, its type, whether it is mandatory and its state; links are drawn from it, see EditionScene.
class InterfaceItem : public QGraphicsItem
{
    Q_DECLARE_TR_FUNCTIONS(InterfaceItem)

  public:
    enum class Kind : std::uint8_t
    {
        Provided,
        Consumed,
    };
    // For qgraphicsitem_cast.
    enum
    {
        Type = UserType + 1
    };

    // What an interface takes of the height of its component.
    static constexpr qreal RowHeight = 20;

    // Its position is the center of its dot, on the edge of parent; its name fits in nameWidth, inside parent.
    InterfaceItem(Kind kind, const Nebula::InterfaceInfo& info, qreal nameWidth, ComponentItem& parent);
    ~InterfaceItem() override;

    [[nodiscard]] int type() const override;
    [[nodiscard]] Kind kind() const;
    [[nodiscard]] const QString& name() const;
    // The component it is an interface of.
    [[nodiscard]] Nebula::Component& component() const;
    // Whether one can be bound to the other: provided and consumed by two components, with the same type.
    [[nodiscard]] bool canLinkTo(const InterfaceItem& other) const;

    // The name of the type of an interface, type being the one of its pointer: QWidget for QWidget*.
    [[nodiscard]] static QString typeName(QMetaType type);

    void setAvailable(bool available);
    // Whether the component needs it and it is bound to nothing: it is drawn in the warning color, and its tooltip
    // tells how to bind it.
    void setMissing(bool missing);
    // Faded, while a link that cannot end on it is being drawn.
    void setDimmed(bool dimmed);

    // The links drawn from it, which follow it and are deleted with it: LinkItem adds and removes itself.
    [[nodiscard]] const QList<LinkItem*>& links() const;
    void addLink(LinkItem& link);
    void removeLink(LinkItem& link);

    [[nodiscard]] QRectF boundingRect() const override;
    // Its dot only, which its tooltip is about and links are drawn from.
    [[nodiscard]] QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  protected:
    // Its tooltip is shown as soon as its dot is hovered.
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    // Tells what it is and its state in its tooltip.
    void refreshToolTip();

    ComponentItem& m_componentItem;
    Kind m_kind;
    QString m_name;
    QMetaType m_type;
    bool m_mandatory;
    qreal m_nameWidth;
    bool m_available = false;
    bool m_missing = false;
    bool m_dimmed = false;
    QList<LinkItem*> m_links;
};
