// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QString>

#include "Core_export.h"

class QObject;

namespace Nebula
{

// An object whose properties are edited and saved with those of its component: see
// Component::addPropertiesForQObject().
struct CORE_EXPORT PropertyObject
{
    QObject* object = nullptr;
    QString name; // Of the group its properties are edited in; with those of the component when empty

    // The properties of object its component answers for: those it declares with Q_PROPERTY, base classes included,
    // but neither objectName, which QObject gives every object, nor those declared DESIGNABLE false or STORED false,
    // which are not meant to be set from outside, as the visibility of a widget.
    [[nodiscard]] QList<QMetaProperty> properties() const;
};

} // namespace Nebula
