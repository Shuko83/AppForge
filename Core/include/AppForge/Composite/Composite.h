// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QString>
#include <QStringView>
#include <memory>
#include <vector>

#include "AppForge/Component/Component.h"
#include "Core_export.h"

namespace AppForge
{

class Assembler;

// A component made of components, as a composite file holds them, see README.md: an application, or another
// composite, places it as many times as it needs, with ForgeApplication::instantiateComposite(), each instance with
// components of its own. Core provides it itself, as a plugin provides its components.
// It initializes, starts and stops its components with itself, in their order, the last one first to be stopped: it
// refuses to be initialized or started as soon as one of them refuses, those it started being stopped again then.
class CORE_EXPORT Composite : public Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Components assembled in a composite file, which run with it")
    Q_CLASSINFO("category", "Composite")

  public:
    ~Composite() override;

    // The composite file it is made from, absolute.
    [[nodiscard]] QString filePath() const;
    // In the order of its file.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components() const;
    // The one of its components named path, or the one path leads to through the composites among them, their names
    // joined with '/': Clock1/Ticker1. nullptr when there is none.
    [[nodiscard]] Component* component(QStringView path) const;
    // The path, from it, of the component that refused the last time it could not be initialized or started.
    [[nodiscard]] QString refusal() const;

  protected:
    Composite();

    bool onInitialize() override;
    bool onStart() override;
    void onStop() override;

  private:
    friend class Assembler; // Fills it

    QString m_filePath;
    std::vector<std::unique_ptr<Component>> m_components;
    QString m_refusal;
};

} // namespace AppForge
