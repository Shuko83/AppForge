#pragma once

#include <QString>
#include <cstdint>

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
};

} // namespace AppForge
