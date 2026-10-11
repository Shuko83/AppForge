#pragma once

#include <QString>

// The interface a Greeter provides while it runs: greets anyone, not only its recipient.
class IGreeter
{
  public:
    virtual ~IGreeter() = default;

    [[nodiscard]] virtual QString greet(const QString& recipient) const = 0;
};
