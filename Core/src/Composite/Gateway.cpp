// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "AppForge/Composite/Gateway.h"

#include <utility>

#include "AppForge/Composite/Composite.h"
#include "CorePlugin.h"

APPFORGE_REGISTER_COMPONENT(AppForge::CorePlugin, AppForge::Gateway);

namespace AppForge
{

Gateway::Gateway() = default;

Gateway::~Gateway()
{
    detach();
}

QString Gateway::interfaceName()
{
    return QStringLiteral("interface");
}

GatewayKind Gateway::kind() const
{
    return m_kind;
}

QMetaType Gateway::interfaceType() const
{
    return m_type;
}

Composite* Gateway::composite() const
{
    return m_composite;
}

void Gateway::attach(Composite& composite, GatewayKind kind, QMetaType type)
{
    m_composite = &composite;
    m_kind = kind;
    m_type = type;
    m_name = objectName();
    // Registered under its name: a composite file names it so, and is read again by its name.
    static_cast<void>(type.id());
    const auto take = [this](void* pointer) { m_pointer = pointer; };
    if(kind == GatewayKind::Provided)
    {
        // What a component of the composite provides to it, the composite provides.
        const auto pass = [this]
        {
            if(m_composite != nullptr)
            {
                m_composite->provide(m_name, m_type, m_pointer);
            }
        };
        const auto withdraw = [this]
        {
            if(m_composite != nullptr)
            {
                m_composite->provide(m_name, m_type, nullptr);
            }
        };
        consume(interfaceName(), type, take, pass, withdraw);
        composite.provide(m_name, type, nullptr);
    }
    else
    {
        // What the composite is given, it provides to components of the composite.
        const auto pass = [this] { provide(interfaceName(), m_type, m_pointer); };
        const auto withdraw = [this] { provide(interfaceName(), m_type, nullptr); };
        provide(interfaceName(), type, nullptr);
        composite.consume(m_name, type, take, pass, withdraw);
    }
}

bool Gateway::rename(const QString& newName)
{
    if(m_composite != nullptr && !m_composite->renameInterface(m_name, newName))
    {
        return false;
    }
    m_name = newName;
    setObjectName(newName);
    return true;
}

void Gateway::detach()
{
    if(Composite* composite = std::exchange(m_composite, nullptr))
    {
        composite->removeInterface(m_name);
    }
}

} // namespace AppForge
