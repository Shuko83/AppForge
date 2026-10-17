// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <cstdint>

#include "AppForge/Component/InterfaceInfo.h"

namespace AppForge
{

// The two modes of a component.
enum class ComponentState : std::uint8_t
{
    Editing, // As ComponentFactory created it, and once stopped: its properties are set and its interfaces bound
    Running,
};

// Contract every component fulfils; derive from Component rather than from this interface directly.
class IComponent
{
  public:
    virtual ~IComponent() = default;

    // <plugin id>.<class name>, see ComponentInfo.
    [[nodiscard]] virtual QString componentId() const = 0;
    [[nodiscard]] virtual ComponentState state() const = 0;

    // Editing -> Running
    virtual bool start() = 0;
    // Running -> Editing
    virtual void stop() = 0;

    // The interfaces it provides, and those it consumes, each in the order it declared them.
    [[nodiscard]] virtual QList<InterfaceInfo> providedInterfaces() const = 0;
    [[nodiscard]] virtual QList<InterfaceInfo> consumedInterfaces() const = 0;

    // The pointer it provides as name; nullptr while it is unavailable, when no interface has this name, or when T is
    // not the type it is provided with.
    template <typename T> [[nodiscard]] T* providedInterface(const QString& name) const
    {
        return static_cast<T*>(providedPointer(name, QMetaType::fromType<T*>()));
    }

  protected:
    // What providedInterface() returns, type being the one of the pointer asked for.
    [[nodiscard]] virtual void* providedPointer(const QString& name, QMetaType type) const = 0;
};

} // namespace AppForge
