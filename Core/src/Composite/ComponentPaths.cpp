// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentPaths.h"

#include <algorithm>

#include "AppForge/Component/Component.h"
#include "AppForge/Composite/Composite.h"

namespace AppForge::ComponentPaths
{

Component* find(const Components& components, QStringView path)
{
    const qsizetype separator = path.indexOf(Separator);
    const QStringView name = separator < 0 ? path : path.first(separator);
    const auto found = std::ranges::find_if(components, [name](const std::unique_ptr<Component>& component)
                                            { return component->objectName() == name; });
    if(found == components.end())
    {
        return nullptr;
    }
    if(separator < 0)
    {
        return found->get();
    }
    const auto* composite = qobject_cast<const Composite*>(found->get());
    return composite != nullptr ? composite->component(path.sliced(separator + 1)) : nullptr;
}

QString path(const Components& components, const Component& component)
{
    for(const std::unique_ptr<Component>& candidate : components)
    {
        if(candidate.get() == &component)
        {
            return candidate->objectName();
        }
        if(const auto* composite = qobject_cast<const Composite*>(candidate.get()))
        {
            const QString inner = path(composite->components(), component);
            if(!inner.isEmpty())
            {
                return candidate->objectName() + Separator + inner;
            }
        }
    }
    return {};
}

bool canName(const Components& components, const QString& name, const Component* named)
{
    if(name.isEmpty() || name.contains(Separator))
    {
        return false;
    }
    return std::ranges::none_of(components, [&name, named](const std::unique_ptr<Component>& component)
                                { return component.get() != named && component->objectName() == name; });
}

QString newName(const Components& components, const QString& base)
{
    for(int number = 1;; ++number)
    {
        const QString name = base + QString::number(number);
        if(canName(components, name))
        {
            return name;
        }
    }
}

QString refusalIn(const Component& refused)
{
    const auto* composite = qobject_cast<const Composite*>(&refused);
    return composite != nullptr ? refused.objectName() + Separator + composite->refusal() : refused.objectName();
}

} // namespace AppForge::ComponentPaths
