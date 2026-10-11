#include "Component/Component.h"

#include <QLoggingCategory>
#include <algorithm>

namespace AppForge
{

namespace
{

Q_LOGGING_CATEGORY(lcComponents, "appforge.components")

} // namespace

Component::Component() = default;

Component::~Component()
{
    // What it still provides ends with it.
    for(ProvidedInterface& provided : m_interfaces)
    {
        if(provided.pointer != nullptr)
        {
            provided.pointer = nullptr;
            emit interfaceUnavailable(provided.name);
        }
    }
}

QString Component::componentId() const
{
    return m_componentId;
}

ComponentState Component::state() const
{
    return m_state;
}

bool Component::initialize()
{
    if(m_state != ComponentState::Initializing || !onInitialize())
    {
        return false;
    }
    setState(ComponentState::Ready);
    return true;
}

bool Component::start()
{
    if(m_state != ComponentState::Ready || !onStart())
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
    setState(ComponentState::Ready);
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

QList<InterfaceInfo> Component::providedInterfaces() const
{
    QList<InterfaceInfo> interfaces;
    interfaces.reserve(m_interfaces.size());
    for(const ProvidedInterface& provided : m_interfaces)
    {
        interfaces.append(
            InterfaceInfo{.name = provided.name, .type = provided.type, .available = provided.pointer != nullptr});
    }
    return interfaces;
}

void* Component::providedPointer(const QString& name, QMetaType type) const
{
    const auto provided = std::ranges::find(m_interfaces, name, &ProvidedInterface::name);
    return provided != m_interfaces.end() && provided->type == type ? provided->pointer : nullptr;
}

bool Component::provide(const QString& name, QMetaType type, void* pointer)
{
    // An index: a slot of the signals emitted here can have the component declare another interface.
    const qsizetype index = std::ranges::find(m_interfaces, name, &ProvidedInterface::name) - m_interfaces.begin();
    if(index == m_interfaces.size())
    {
        m_interfaces.append(ProvidedInterface{.name = name, .type = type});
        emit interfaceAdded(name);
    }
    else if(m_interfaces.at(index).type != type)
    {
        qCWarning(lcComponents).noquote() << "Interface" << name << "cannot be provided as" << type.name()
                                          << "- already provided as" << m_interfaces.at(index).type.name();
        return false;
    }

    if(m_interfaces.at(index).pointer == pointer)
    {
        return true;
    }
    if(m_interfaces.at(index).pointer != nullptr)
    {
        m_interfaces[index].pointer = nullptr;
        emit interfaceUnavailable(name);
    }
    if(pointer != nullptr)
    {
        m_interfaces[index].pointer = pointer;
        emit interfaceAvailable(name);
    }
    return true;
}

void Component::setState(ComponentState state)
{
    m_state = state;
    emit stateChanged(state);
}

} // namespace AppForge
