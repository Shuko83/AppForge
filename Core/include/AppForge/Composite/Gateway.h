// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMetaType>
#include <QString>
#include <cstdint>

#include "AppForge/Component/Component.h"
#include "Core_export.h"

namespace AppForge
{

class Composite;

// Which way a gateway passes its interface, from the point of view of the scene its composite is placed in.
enum class GatewayKind : std::uint8_t
{
    Provided, // The composite provides it: a component of the composite provides it to the gateway
    Consumed, // The composite consumes it: the gateway provides it to components of the composite
};

// Where a composite meets the scene it is placed in: one of the components of the composite, which is an interface of
// the composite, named as it is. A provided gateway consumes, among the components of the composite, the interface the
// composite provides; a consumed one provides, to components of the composite, the interface the composite consumes.
// The pointer passes as it is, and so does its availability: when the one that provides it withdraws it, those that
// consume it on the other side are told before it goes. Core provides it, as it provides Composite, and
// ForgeApplication::expose() adds one to the structure of a composite file.
class CORE_EXPORT Gateway : public Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Passes an interface between a composite and its components")
    Q_CLASSINFO("category", "Composite")

  public:
    // Detached from its composite: the composite no longer has its interface.
    ~Gateway() override;

    // The name of its own interface, which components of its composite are bound to: the one it consumes when it is
    // provided, or provides when it is consumed. Its type is interfaceType().
    [[nodiscard]] static QString interfaceName();

    [[nodiscard]] GatewayKind kind() const;
    // The type of the interface it passes, the pointer to the class: QWidget* for a QWidget.
    [[nodiscard]] QMetaType interfaceType() const;
    // The composite whose interface it is; nullptr before it is the one of a composite.
    [[nodiscard]] Composite* composite() const;

  protected:
    Gateway();

  private:
    friend class Assembler;        // Attaches the gateways it reads
    friend class Composite;        // Detaches them before it goes
    friend class ForgeApplication; // Attaches, renames and detaches them as it changes a composite

    // Declares its interface, of type, as kind tells, and the one of composite, named as it is: from then on, what one
    // of them is given, the other passes on.
    void attach(Composite& composite, GatewayKind kind, QMetaType type);
    // Names it, and the interface of its composite, newName; false, changing nothing, when the composite has an
    // interface of that name already.
    bool rename(const QString& newName);
    // Removes the interface of its composite, and what was bound to it: it is not its composite's any longer.
    void detach();

    GatewayKind m_kind = GatewayKind::Provided;
    QMetaType m_type;
    QString m_name;                   // The one of the interface of its composite
    Composite* m_composite = nullptr; // nullptr until it is attached, and once it is detached
    void* m_pointer = nullptr;        // What it passes, while it has it
};

} // namespace AppForge
