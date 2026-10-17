// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsObject>
#include <QList>
#include <QString>
#include <QStringList>

class InterfaceItem;
class NameEditor;

namespace Nebula
{
class Component;
}

// A component instantiated in the edition zone: its name above the name of its component, then its interfaces, one
// InterfaceItem per row: those it consumes on its left edge, then those it provides on its right edge. A triangle in
// its top right corner tells that it runs. An exclamation mark in its top left corner tells that it does not work: an
// interface it cannot do without is bound to nothing, see Component::unboundMandatoryInterfaces(); its tooltip names
// them and tells how to bind them. It can be selected, and, while it is editable, moved, and its name edited in place
// by double-clicking its names. It can be given a color, which its names are written over.
class ComponentItem : public QGraphicsObject
{
    Q_OBJECT

  public:
    // component, owned by the application, outlives the item.
    ComponentItem(Nebula::Component& component, QString componentName);

    [[nodiscard]] Nebula::Component& component() const;
    // Its interfaces, in the order of Component::consumedInterfaces() and Component::providedInterfaces().
    [[nodiscard]] const QList<InterfaceItem*>& consumedInterfaces() const;
    [[nodiscard]] const QList<InterfaceItem*>& providedInterfaces() const;
    // Not valid until it is given one: it has the colors of the style. As opaque as it was chosen.
    [[nodiscard]] const QColor& color() const;
    void setColor(const QColor& color);
    // Edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. It
    // must be in a scene. See nameEdited().
    void editName();
    // Whether it can be moved, and its name edited by double-clicking it: it is until told otherwise.
    void setEditable(bool editable);

    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  signals:
    // name was typed in place of its name, on one line: its component is still to be named so.
    void nameEdited(const QString& name);

  protected:
    // Not moved while its name is edited: the double-click that edits it still holds it, and the view scrolling to
    // show what is typed would drag it along.
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

  private:
    // Whether its name is being edited in place.
    [[nodiscard]] bool isNameEdited() const;
    // Shows the interfaces of the component as they are now: it only ever declares more of them. Then whether it
    // works.
    void refreshInterfaces();

    Nebula::Component& m_component;
    QString m_componentName;
    QStringList m_unbound; // Component::unboundMandatoryInterfaces(), as refreshInterfaces() last read them
    QColor m_color;
    bool m_editable = true;
    // Its children.
    QList<InterfaceItem*> m_consumedInterfaces;
    QList<InterfaceItem*> m_providedInterfaces;
    NameEditor* m_nameEditor = nullptr; // Over its name while it is edited; created when first so
};
