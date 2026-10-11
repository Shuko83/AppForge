#pragma once

#include <QObject>

#include "Component/IComponent.h"
#include "IGreeter.h"

class Greeter;

// Logic of a Greeter, built with it by ExamplePlugin: writes the greeting of its recipient when the Greeter starts,
// and is the IGreeter it provides while it runs.
class GreeterLogic : public QObject, public IGreeter
{
    Q_OBJECT

  public:
    // A child of greeter, destroyed with it.
    explicit GreeterLogic(Greeter& greeter);

    [[nodiscard]] QString greet(const QString& recipient) const override;

  private:
    void onStateChanged(AppForge::ComponentState state);

    Greeter& m_greeter;
};
