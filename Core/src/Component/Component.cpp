#include "Component/Component.h"

#include <utility>

namespace AppForge
{

Component::Component(QString name, QObject* parent) : QObject(parent), m_name(std::move(name)) {}

Component::~Component() = default;

QString Component::name() const
{
    return m_name;
}

ComponentState Component::state() const
{
    return m_state;
}

bool Component::initialize()
{
    if(m_state != ComponentState::Created || !onInitialize())
    {
        return false;
    }
    setState(ComponentState::Initialized);
    return true;
}

bool Component::start()
{
    if((m_state != ComponentState::Initialized && m_state != ComponentState::Stopped) || !onStart())
    {
        return false;
    }
    setState(ComponentState::Running);
    return true;
}

void Component::stop()
{
    if(m_state != ComponentState::Running)
    {
        return;
    }
    onStop();
    setState(ComponentState::Stopped);
}

bool Component::onInitialize()
{
    return true;
}

bool Component::onStart()
{
    return true;
}

void Component::onStop() {}

void Component::setState(ComponentState state)
{
    m_state = state;
    emit stateChanged(state);
}

} // namespace AppForge
