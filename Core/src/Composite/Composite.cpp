// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "AppForge/Composite/Composite.h"

#include <algorithm>
#include <ranges>

#include "AppForge/Composite/Gateway.h"
#include "Application/ProjectJson.h"

#include "ComponentPaths.h"
#include "CorePlugin.h"

APPFORGE_REGISTER_COMPONENT(AppForge::CorePlugin, AppForge::Composite);

namespace AppForge
{

Composite::Composite() = default;

Composite::~Composite()
{
    // Its gateways no longer pass anything to it: it goes.
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(auto* gateway = qobject_cast<Gateway*>(component.get()))
        {
            gateway->m_composite = nullptr;
        }
    }
    // The last one first, as they are stopped: one can consume what one before it provides.
    while(!m_components.empty())
    {
        m_components.pop_back();
    }
}

QString Composite::filePath() const
{
    return m_filePath;
}

const std::vector<std::unique_ptr<Component>>& Composite::components() const
{
    return m_components;
}

Component* Composite::component(QStringView path) const
{
    return ComponentPaths::find(m_components, path);
}

QString Composite::refusal() const
{
    return m_refusal;
}

const QList<ExposedProperty>& Composite::exposedProperties() const
{
    return m_exposed;
}

ExposedTarget Composite::exposedProperty(const QString& name) const
{
    const auto exposed = std::ranges::find(m_exposed, name, &ExposedProperty::name);
    // One of its own components only: what is exposed is of the composite it is in.
    const auto owner = exposed != m_exposed.end() ? std::ranges::find(m_components, exposed->component,
                                                                      [](const std::unique_ptr<Component>& component)
                                                                      { return component->objectName(); })
                                                  : m_components.end();
    if(owner == m_components.end())
    {
        return {};
    }
    Component& component = **owner;
    if(!exposed->object)
    {
        // What a composite exposes in turn, or a property of the component.
        if(const auto* composite = qobject_cast<const Composite*>(&component))
        {
            return composite->exposedProperty(exposed->property);
        }
        const QMetaObject* metaObject = component.metaObject();
        return {.object = &component,
                .component = &component,
                .property = metaObject->property(metaObject->indexOfProperty(exposed->property.toLatin1()))};
    }
    QObject* object =
        ProjectJson::object(component, ProjectJson::ObjectKey{.name = *exposed->object, .index = exposed->index});
    if(object == nullptr)
    {
        return {};
    }
    const QMetaObject* metaObject = object->metaObject();
    return {.object = object,
            .component = &component,
            .property = metaObject->property(metaObject->indexOfProperty(exposed->property.toLatin1()))};
}

bool Composite::onInitialize()
{
    // All of them, as an application initializes its components: one that starts can count on the others being ready.
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(component->state() == ComponentState::Initializing && !component->initialize())
        {
            m_refusal = ComponentPaths::refusalIn(*component);
            return false;
        }
    }
    m_refusal.clear();
    return true;
}

bool Composite::onStart()
{
    // Those added since it was initialized as well.
    if(!onInitialize())
    {
        return false;
    }
    std::vector<Component*> started;
    for(const std::unique_ptr<Component>& component : m_components)
    {
        if(component->state() != ComponentState::Ready)
        {
            continue; // It runs already
        }
        if(!component->start())
        {
            m_refusal = ComponentPaths::refusalIn(*component);
            for(Component* running : started | std::views::reverse)
            {
                running->stop();
            }
            return false;
        }
        started.push_back(component.get());
    }
    m_refusal.clear();
    return true;
}

void Composite::onStop()
{
    for(const std::unique_ptr<Component>& component : m_components | std::views::reverse)
    {
        component->stop(); // Does nothing to one that does not run
    }
}

} // namespace AppForge
