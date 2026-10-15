// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMetaProperty>
#include <QString>
#include <QVariant>
#include <optional>

class QObject;

namespace AppForge
{
class Component;
} // namespace AppForge

// How a project, and a settings file, hold the properties of the components and of their objects: shared by
// ForgeApplication and SettingsFile.
namespace AppForge::ProjectJson
{

// value as JSON holds it; nullopt when it would not be read back as it is, its type being one JSON has nothing for.
[[nodiscard]] std::optional<QJsonValue> toJson(const QVariant& value);

// The properties of object JSON holds, by name: those of properties that can be written, and whose value toJson()
// holds.
[[nodiscard]] QJsonObject writableProperties(const QObject& object, const QList<QMetaProperty>& properties);

// Sets the property name of object, one of properties, to value; false when it is none of them or cannot be set.
bool writeProperty(QObject& object, const QList<QMetaProperty>& properties, const QString& name,
                   const QJsonValue& value);

// An object a component added with Component::addPropertiesForQObject(), as JSON finds it again: the name it was
// added with, and how many of the objects of the component were added with that name before it.
struct ObjectKey
{
    QString name;
    qsizetype index = 0;
};

// The key of object among the objects of component; nullopt when it is none of them.
[[nodiscard]] std::optional<ObjectKey> objectKey(const Component& component, const QObject* object);
// The object of component key is the key of; nullptr when it has none.
[[nodiscard]] QObject* object(const Component& component, const ObjectKey& key);

// The properties a project holds for object, component itself or one of its objects, when they can be written and
// JSON holds their value: those ComponentFactory knows of a component, those PropertyObject::properties() lists of
// an object. None when object is neither.
[[nodiscard]] QList<QMetaProperty> savedProperties(const Component& component, const QObject* object);

} // namespace AppForge::ProjectJson
