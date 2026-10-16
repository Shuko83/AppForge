// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "AppForge/Component/Component.h"

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

QList<InterfaceInfo> Component::providedInterfaces() const
{
    QList<InterfaceInfo> interfaces;
    interfaces.reserve(m_providedInterfaces.size());
    for(const ProvidedInterface& provided : m_providedInterfaces)
    {
        interfaces.append(
            InterfaceInfo{.name = provided.name, .type = provided.type, .available = provided.pointer != nullptr});
    }
    return interfaces;
}

QList<InterfaceInfo> Component::consumedInterfaces() const
{
    QList<InterfaceInfo> interfaces;
    interfaces.reserve(m_consumedInterfaces.size());
    for(const ConsumedInterface& consumed : m_consumedInterfaces)
    {
        interfaces.append(InterfaceInfo{.name = consumed.name, .type = consumed.type, .available = consumed.available});
    }
    return interfaces;
}

bool Component::bindInterface(const QString& name, Component& provider, const QString& providedName)
{
    const auto consumed = std::ranges::find(std::as_const(m_consumedInterfaces), name, &ConsumedInterface::name);
    const auto provided =
        std::ranges::find(std::as_const(provider.m_providedInterfaces), providedName, &ProvidedInterface::name);
    // Not to itself either: it would be told of its own destruction once its interfaces are gone.
    if(&provider == this || consumed == m_consumedInterfaces.cend() ||
       provided == provider.m_providedInterfaces.cend() || consumed->type != provided->type)
    {
        return false;
    }
    unbindInterface(name);

    // Found again: the callback unbindInterface() called can have the component declare another interface.
    ConsumedInterface& bound = *std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    bound.provider = &provider;
    bound.providedName = providedName;
    followProvider(name);
    deliverInterface(name);
    return true;
}

void Component::followProvider(const QString& name)
{
    ConsumedInterface& bound = *std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    // Whether provided is what name is bound to: read when the provider tells, as a composite can rename it meanwhile.
    const auto isBound = [this, name](const QString& provided)
    {
        const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
        return consumed != m_consumedInterfaces.end() && consumed->providedName == provided;
    };
    bound.connections = {
        connect(bound.provider, &Component::interfaceAvailable, this,
                [this, name, isBound](const QString& available)
                {
                    if(isBound(available))
                    {
                        deliverInterface(name);
                    }
                }),
        connect(bound.provider, &Component::interfaceUnavailable, this,
                [this, name, isBound](const QString& unavailable)
                {
                    if(isBound(unavailable))
                    {
                        withdrawInterface(name);
                    }
                }),
        connect(bound.provider, &Component::interfaceRenamed, this,
                [this, name, isBound](const QString& former, const QString& renamed)
                {
                    if(isBound(former))
                    {
                        std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name)->providedName = renamed;
                    }
                }),
        connect(bound.provider, &Component::interfaceRemoved, this,
                [this, name, isBound](const QString& removed)
                {
                    if(isBound(removed))
                    {
                        unbindInterface(name);
                    }
                }),
        connect(bound.provider, &QObject::destroyed, this, [this, name] { unbindInterface(name); }),
    };
}

bool Component::renameInterface(const QString& name, const QString& newName)
{
    const auto provided = std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name);
    if(provided != m_providedInterfaces.end())
    {
        if(newName == name)
        {
            return true;
        }
        if(std::ranges::find(m_providedInterfaces, newName, &ProvidedInterface::name) != m_providedInterfaces.end())
        {
            return false;
        }
        provided->name = newName;
        emit interfaceRenamed(name, newName);
        emit interfacesChanged();
        return true;
    }
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end() ||
       (newName != name &&
        std::ranges::find(m_consumedInterfaces, newName, &ConsumedInterface::name) != m_consumedInterfaces.end()))
    {
        return false;
    }
    if(newName == name)
    {
        return true;
    }
    // Its connections to its provider know it by its name.
    for(const QMetaObject::Connection& connection : std::as_const(consumed->connections))
    {
        disconnect(connection);
    }
    consumed->connections.clear();
    consumed->name = newName;
    if(consumed->provider != nullptr)
    {
        followProvider(newName);
    }
    emit interfacesChanged();
    return true;
}

void Component::removeInterface(const QString& name)
{
    const qsizetype provided =
        std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name) - m_providedInterfaces.begin();
    if(provided < m_providedInterfaces.size())
    {
        // Told unavailable first: what is bound to it lets go of its pointer while it is still valid.
        if(m_providedInterfaces.at(provided).pointer != nullptr)
        {
            m_providedInterfaces[provided].pointer = nullptr;
            emit interfaceUnavailable(name);
        }
        // Found again: what was told can have the component declare another interface.
        m_providedInterfaces.removeIf([&name](const ProvidedInterface& declared) { return declared.name == name; });
        emit interfaceRemoved(name);
        emit interfacesChanged();
        return;
    }
    if(std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name) != m_consumedInterfaces.end())
    {
        unbindInterface(name);
        m_consumedInterfaces.removeIf([&name](const ConsumedInterface& declared) { return declared.name == name; });
        emit interfacesChanged();
    }
}

void Component::unbindInterface(const QString& name)
{
    withdrawInterface(name);
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return;
    }
    for(const QMetaObject::Connection& connection : std::as_const(consumed->connections))
    {
        disconnect(connection);
    }
    consumed->connections.clear();
    consumed->provider = nullptr;
    consumed->providedName.clear();
}

InterfaceBinding Component::binding(const QString& name) const
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end())
    {
        return {};
    }
    return {.provider = consumed->provider, .providedName = consumed->providedName};
}

void Component::addPropertiesForQObject(QObject* object, const QString& name)
{
    if(object == nullptr || object == this ||
       std::ranges::find(m_propertyObjects, object, &PropertyObject::object) != m_propertyObjects.end())
    {
        qCWarning(lcComponents).noquote() << "The properties of" << object << "cannot be added to" << objectName();
        return;
    }
    m_propertyObjects.append(PropertyObject{.object = object, .name = name});
    connect(object, &QObject::destroyed, this,
            [this, object]
            {
                m_propertyObjects.removeIf([object](const PropertyObject& added) { return added.object == object; });
                emit propertyObjectsChanged();
            });
    emit propertyObjectsChanged();
}

QList<PropertyObject> Component::propertyObjects() const
{
    return m_propertyObjects;
}

void* Component::providedPointer(const QString& name, QMetaType type) const
{
    const auto provided = std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name);
    return provided != m_providedInterfaces.end() && provided->type == type ? provided->pointer : nullptr;
}

bool Component::provide(const QString& name, QMetaType type, void* pointer)
{
    // An index: a slot of the signals emitted here can have the component declare another interface.
    const qsizetype index =
        std::ranges::find(m_providedInterfaces, name, &ProvidedInterface::name) - m_providedInterfaces.begin();
    const bool declared = index == m_providedInterfaces.size();
    if(declared)
    {
        m_providedInterfaces.append(ProvidedInterface{.name = name, .type = type});
    }
    else if(m_providedInterfaces.at(index).type != type)
    {
        qCWarning(lcComponents).noquote() << "Interface" << name << "cannot be provided as" << type.name()
                                          << "- already provided as" << m_providedInterfaces.at(index).type.name();
        return false;
    }

    const bool replaced = m_providedInterfaces.at(index).pointer != pointer;
    if(replaced && m_providedInterfaces.at(index).pointer != nullptr)
    {
        m_providedInterfaces[index].pointer = nullptr;
        emit interfaceUnavailable(name);
    }
    if(replaced && pointer != nullptr)
    {
        m_providedInterfaces[index].pointer = pointer;
        emit interfaceAvailable(name);
    }
    if(declared || replaced)
    {
        emit interfacesChanged();
    }
    return true;
}

bool Component::consume(const QString& name, QMetaType type, std::function<void(void*)> setMember,
                        std::function<void()> onAvailable, std::function<void()> onUnavailable)
{
    if(std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name) != m_consumedInterfaces.end())
    {
        qCWarning(lcComponents).noquote() << "Interface" << name << "is already consumed";
        return false;
    }
    setMember(nullptr);
    m_consumedInterfaces.append(ConsumedInterface{.name = name,
                                                  .type = type,
                                                  .setMember = std::move(setMember),
                                                  .onAvailable = std::move(onAvailable),
                                                  .onUnavailable = std::move(onUnavailable)});
    emit interfacesChanged();
    return true;
}

void Component::deliverInterface(const QString& name)
{
    const auto consumed = std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name);
    if(consumed == m_consumedInterfaces.end() || consumed->available || consumed->provider == nullptr)
    {
        return;
    }
    void* pointer = consumed->provider->providedPointer(consumed->providedName, consumed->type);
    if(pointer == nullptr)
    {
        return;
    }
    consumed->available = true;
    consumed->setMember(pointer);
    // A copy: it can have the component declare another interface.
    if(const std::function<void()> onAvailable = consumed->onAvailable)
    {
        onAvailable();
    }
    emit interfacesChanged();
}

void Component::withdrawInterface(const QString& name)
{
    // An index and a copy: onUnavailable can have the component declare another interface.
    const qsizetype index =
        std::ranges::find(m_consumedInterfaces, name, &ConsumedInterface::name) - m_consumedInterfaces.begin();
    if(index == m_consumedInterfaces.size() || !m_consumedInterfaces.at(index).available)
    {
        return;
    }
    // Unavailable from now on, should onUnavailable unbind it; the member still points to what it has to let go of.
    m_consumedInterfaces[index].available = false;
    if(const std::function<void()> onUnavailable = m_consumedInterfaces.at(index).onUnavailable)
    {
        onUnavailable();
    }
    m_consumedInterfaces.at(index).setMember(nullptr);
    emit interfacesChanged();
}

void Component::releaseInterfaces()
{
    // By index, with copies of the names: what is called here can have the component declare another interface.
    for(qsizetype index = 0; index < m_consumedInterfaces.size(); ++index)
    {
        const QString name = m_consumedInterfaces.at(index).name;
        unbindInterface(name);
    }
    for(qsizetype index = 0; index < m_providedInterfaces.size(); ++index)
    {
        if(m_providedInterfaces.at(index).pointer != nullptr)
        {
            const QString name = m_providedInterfaces.at(index).name;
            m_providedInterfaces[index].pointer = nullptr;
            emit interfaceUnavailable(name);
        }
    }
}

void Component::setState(ComponentState state)
{
    m_state = state;
    emit stateChanged(state);
}

} // namespace AppForge
