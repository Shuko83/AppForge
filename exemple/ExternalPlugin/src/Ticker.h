#pragma once

#include "Component/Component.h"

// Exposes the ticks of a timer while it runs, counted by TickerLogic, which ExternalPlugin builds it with.
class Ticker : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Counts the ticks of a timer while it runs")
    Q_CLASSINFO("category", "Example")
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(int ticks READ ticks NOTIFY ticksChanged)

  public:
    // In milliseconds.
    [[nodiscard]] int interval() const;
    void setInterval(int interval);

    [[nodiscard]] int ticks() const;
    // Not a property setter: only its logic counts the ticks.
    void setTicks(int ticks);

  signals:
    void intervalChanged(int interval);
    void ticksChanged(int ticks);

  private:
    int m_interval = 1000;
    int m_ticks = 0;
};
