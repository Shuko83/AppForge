#include "Assembly.h"

#include <algorithm>
#include <optional>

#include "AppForge/Component/ComponentFactory.h"

Assembly::Assembly(const AppForge::ComponentFactory& factory) : m_factory(factory) {}

Assembly::~Assembly() = default;

AppForge::Component* Assembly::instantiate(const QString& componentId, const QString& name)
{
    const std::optional<AppForge::ComponentInfo> info = m_factory.component(componentId);
    std::unique_ptr<AppForge::Component> component = m_factory.create(componentId);
    if(!info || component == nullptr)
    {
        return nullptr;
    }
    component->setObjectName(name.isEmpty() || contains(name) ? newName(info->name) : name);
    return m_components.emplace_back(std::move(component)).get();
}

const std::vector<std::unique_ptr<AppForge::Component>>& Assembly::components() const
{
    return m_components;
}

void Assembly::clear()
{
    m_components.clear();
}

bool Assembly::contains(const QString& name) const
{
    return std::ranges::any_of(m_components, [&](const auto& component) { return component->objectName() == name; });
}

QString Assembly::newName(const QString& componentName) const
{
    for(int number = 1;; ++number)
    {
        const QString name = componentName + QString::number(number);
        if(!contains(name))
        {
            return name;
        }
    }
}
