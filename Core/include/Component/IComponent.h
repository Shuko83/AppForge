#pragma once

#include <QString>
#include <cstdint>

namespace AppForge
{

enum class ComponentState : std::uint8_t
{
    Created,
    Initialized,
    Running,
    Stopped,
};

// Contract every component fulfils; derive from Component rather than from this interface directly.
class IComponent
{
  public:
    virtual ~IComponent() = default;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual ComponentState state() const = 0;

    // Created -> Initialized
    virtual bool initialize() = 0;
    // Initialized or Stopped -> Running
    virtual bool start() = 0;
    // Running -> Stopped
    virtual void stop() = 0;
};

} // namespace AppForge
