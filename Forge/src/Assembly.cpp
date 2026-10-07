#include "Assembly.h"

#include <algorithm>
#include <optional>

#include "Component/ComponentFactory.h"

Assembly::Assembly(const AppForge::ComponentFactory& factory) : m_factory(factory) {}

Assembly::~Assembly() = default;

AppForge::Component* Assembly::instantiate(const QString& componentId)
{
    const std::optional<AppForge::ComponentInfo> info = m_factory.component(componentId);
    std::unique_ptr<AppForge::Component> component = m_factory.create(componentId);
    if(!info || component == nullptr)
    {
        return nullptr;
    }
    component->setObjectName(newName(info->name));
    return m_components.emplace_back(std::move(component)).get();
}

QString Assembly::newName(const QString& componentName) const
{
    for(int number = 1;; ++number)
    {
        const QString name = componentName + QString::number(number);
        if(std::ranges::none_of(m_components, [&](const auto& component) { return component->objectName() == name; }))
        {
            return name;
        }
    }
}
