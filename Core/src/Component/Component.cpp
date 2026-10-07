#include "Component/Component.h"

#include <QLoggingCategory>
#include <algorithm>
#include <utility>

namespace AppForge
{

namespace
{

Q_LOGGING_CATEGORY(lcComponents, "appforge.components")

} // namespace

Component::Component() = default;

Component::~Component() = default;

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

QStringList Component::providedInterfaces() const
{
    QStringList ids;
    for(const ProvidedInterface& provided : m_providedInterfaces)
    {
        ids.append(provided.id);
    }
    return ids;
}

QStringList Component::consumedInterfaces() const
{
    QStringList ids;
    for(const ConsumedInterface& consumed : m_consumedInterfaces)
    {
        ids.append(consumed.id);
    }
    return ids;
}

bool Component::bindInterface(const QString& interfaceId, const Component& provider)
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, interfaceId, &ConsumedInterface::id);
    const auto provided = std::ranges::find(provider.m_providedInterfaces, interfaceId, &ProvidedInterface::id);
    if(consumed == m_consumedInterfaces.end() || provided == provider.m_providedInterfaces.end())
    {
        return false;
    }
    disconnect(consumed->providerDestroyed);
    consumed->setter(provided->getter());
    consumed->provider = &provider;
    consumed->providerDestroyed =
        connect(&provider, &QObject::destroyed, this, [this, interfaceId] { unbindInterface(interfaceId); });
    emit consumedInterfaceChanged(interfaceId);
    return true;
}

void Component::unbindInterface(const QString& interfaceId)
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, interfaceId, &ConsumedInterface::id);
    if(consumed == m_consumedInterfaces.end() || consumed->provider == nullptr)
    {
        return;
    }
    disconnect(consumed->providerDestroyed);
    consumed->setter(nullptr);
    consumed->provider = nullptr;
    emit consumedInterfaceChanged(interfaceId);
}

void Component::setState(ComponentState state)
{
    m_state = state;
    emit stateChanged(state);
}

void Component::addProvidedInterface(const QString& id, Getter getter)
{
    if(std::ranges::find(m_providedInterfaces, id, &ProvidedInterface::id) != m_providedInterfaces.end())
    {
        qCWarning(lcComponents).noquote() << metaObject()->className() << "already provides" << id;
        return;
    }
    m_providedInterfaces.push_back(ProvidedInterface{.id = id, .getter = std::move(getter)});
}

void Component::addConsumedInterface(const QString& id, Setter setter)
{
    if(std::ranges::find(m_consumedInterfaces, id, &ConsumedInterface::id) != m_consumedInterfaces.end())
    {
        qCWarning(lcComponents).noquote() << metaObject()->className() << "already consumes" << id;
        return;
    }
    m_consumedInterfaces.push_back(ConsumedInterface{.id = id, .setter = std::move(setter)});
}

} // namespace AppForge
