// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QPointer>
#include <QString>
#include <QStringView>
#include <memory>
#include <optional>
#include <vector>

#include "AppForge/Component/Component.h"
#include "Core_export.h"

namespace AppForge
{

class Assembler;

// A property a composite exposes, as its file holds it: one of one of its components, or of one of the objects of that
// component, see Component::propertyObjects(), or one a composite among its components exposes in turn.
struct ExposedProperty
{
    QString name;                  // What the composite exposes it as: no other property it exposes has that name
    QString component;             // The name of its component in the composite
    std::optional<QString> object; // The name of its object, when it is not the component's own
    qsizetype index = 0;           // Of its object among those of that name, from 0
    QString property;              // Its name on its object, or the name the composite component exposes it as

    [[nodiscard]] bool operator==(const ExposedProperty& other) const = default;
};

// Where a property a composite exposes is: the same property, on the same object, as on its component.
struct ExposedTarget
{
    QPointer<QObject> object;      // The component, or one of its objects
    QPointer<Component> component; // The one of the composite, or of one it holds, the property is of
    QMetaProperty property;

    [[nodiscard]] bool isValid() const
    {
        return object != nullptr && property.isValid();
    }
};

// A component made of components, as a composite file holds them, see README.md: an application, or another
// composite, places it as many times as it needs, with ForgeApplication::instantiateComposite(), each instance with
// components of its own. Core provides it itself, as a plugin provides its components.
// It initializes, starts and stops its components with itself, in their order, the last one first to be stopped: it
// refuses to be initialized or started as soon as one of them refuses, those it started being stopped again then. One
// added to its structure while it runs waits until ForgeApplication::start() starts it.
// It exposes properties of its components, which are edited on it as they are on them: the same properties, on the
// same objects. Which ones is part of its structure, see ForgeApplication::exposeProperty().
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

    // In the order they were exposed.
    [[nodiscard]] const QList<ExposedProperty>& exposedProperties() const;
    // Where the property it exposes as name is, through the composites that expose it in turn; not valid when it
    // exposes none of that name, or what it exposes is no longer there.
    [[nodiscard]] ExposedTarget exposedProperty(const QString& name) const;

  signals:
    // It exposes another property, or no longer one, or one under another name.
    void exposedPropertiesChanged();

  protected:
    Composite();

    bool onInitialize() override;
    bool onStart() override;
    void onStop() override;

  private:
    friend class Assembler;        // Fills it
    friend class ForgeApplication; // Changes its structure

    QString m_filePath;
    std::vector<std::unique_ptr<Component>> m_components;
    QList<ExposedProperty> m_exposed;
    QString m_refusal;
};

} // namespace AppForge
