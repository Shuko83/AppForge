// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QStringView>
#include <memory>
#include <vector>

namespace AppForge
{
class Component;
} // namespace AppForge

// How the components of an application, or of a composite, are named: each by a name of its own among them, and those
// of a composite they hold by a path, the names that lead to it joined with '/': Clock1/Ticker1. Shared by
// ForgeApplication and Composite.
namespace AppForge::ComponentPaths
{

using Components = std::vector<std::unique_ptr<Component>>;

// What joins the names of a path.
constexpr QChar Separator = u'/';

// The one of components path names, or leads to through the composites among them; nullptr when there is none.
[[nodiscard]] Component* find(const Components& components, QStringView path);
// The path of component among components, through the composites among them; empty when it is none of them.
[[nodiscard]] QString path(const Components& components, const Component& component);
// Whether name can name named, one of components or one about to be: it is not empty, holds no separator, and no other
// one of components is named so.
[[nodiscard]] bool canName(const Components& components, const QString& name, const Component* named = nullptr);
// base and the first number from 1 none of components is named with: Ticker1, Ticker2...
[[nodiscard]] QString newName(const Components& components, const QString& base);
// The path of what refused in refused, which just refused to be initialized or started, from where refused is one of
// the components: its name, or its path to one of its components when it is a composite, see Composite::refusal().
[[nodiscard]] QString refusalIn(const Component& refused);

} // namespace AppForge::ComponentPaths
