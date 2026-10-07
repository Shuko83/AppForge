#include "GreeterLogic.h"

#include "Greeter.h"

GreeterLogic::GreeterLogic(Greeter& greeter) : QObject(&greeter), m_greeter(greeter)
{
    connect(&m_greeter, &AppForge::Component::stateChanged, this, &GreeterLogic::onStateChanged);
}

void GreeterLogic::onStateChanged(AppForge::ComponentState state)
{
    if(state == AppForge::ComponentState::Running)
    {
        m_greeter.setGreeting(QStringLiteral("Hello, %1!").arg(m_greeter.recipient()));
    }
}
