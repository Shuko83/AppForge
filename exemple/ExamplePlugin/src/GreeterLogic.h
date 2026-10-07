#pragma once

#include <QObject>

#include "Component/IComponent.h"

class Greeter;

// Logic of a Greeter, built with it by ExamplePlugin: writes the greeting of its recipient when the Greeter starts.
class GreeterLogic : public QObject
{
    Q_OBJECT

  public:
    // A child of greeter, destroyed with it.
    explicit GreeterLogic(Greeter& greeter);

  private:
    void onStateChanged(AppForge::ComponentState state);

    Greeter& m_greeter;
};
