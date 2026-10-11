#pragma once

#include <QList>
#include <QMetaType>
#include <QString>
#include <cstdint>

#include "Component/InterfaceInfo.h"

namespace AppForge
{

enum class ComponentState : std::uint8_t
{
    Initializing, // Created by ComponentFactory: its properties can be set
    Ready,        // Initialized
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

    // Initializing -> Ready
    virtual bool initialize() = 0;
    // Ready -> Running
    virtual bool start() = 0;
    // Running -> Ready
    virtual void stop() = 0;

    // The interfaces it provides, in the order it declared them.
    [[nodiscard]] virtual QList<InterfaceInfo> providedInterfaces() const = 0;

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
