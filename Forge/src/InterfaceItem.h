#pragma once

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QList>
#include <QMetaType>
#include <QString>
#include <cstdint>

class ComponentItem;
class LinkItem;

namespace AppForge
{
class Component;
struct InterfaceInfo;
} // namespace AppForge

// An interface of a component, on an edge of its ComponentItem: a dot, with its name next to it, inside the component.
// One the component provides is on its right edge: a disc while it is available, a dotted circle otherwise. One it
// consumes is on its left edge: a circle. Hovering the dot tells, in its tooltip, whether the interface is provided or
// consumed, its type and its state; links are drawn from it, see EditionScene.
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
    InterfaceItem(Kind kind, const AppForge::InterfaceInfo& info, qreal nameWidth, ComponentItem& parent);
    ~InterfaceItem() override;

    [[nodiscard]] int type() const override;
    [[nodiscard]] Kind kind() const;
    [[nodiscard]] const QString& name() const;
    // The component it is an interface of.
    [[nodiscard]] AppForge::Component& component() const;
    // Whether one can be bound to the other: provided and consumed by two components, with the same type.
    [[nodiscard]] bool canLinkTo(const InterfaceItem& other) const;

    void setAvailable(bool available);
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
    ComponentItem& m_componentItem;
    Kind m_kind;
    QString m_name;
    QMetaType m_type;
    qreal m_nameWidth;
    bool m_available = false;
    bool m_dimmed = false;
    QList<LinkItem*> m_links;
};
