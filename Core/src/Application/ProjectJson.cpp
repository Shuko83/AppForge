// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProjectJson.h"

#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QObject>
#include <algorithm>
#include <memory>

#include "AppForge/Component/Component.h"
#include "AppForge/Component/ComponentFactory.h"
#include "AppForge/Composite/Composite.h"
#include "AppForge/Composite/Gateway.h"

namespace AppForge::ProjectJson
{

std::optional<QJsonValue> toJson(const QVariant& value)
{
    const QJsonValue json = QJsonValue::fromVariant(value);
    QVariant read = json.toVariant();
    if(!read.convert(value.metaType()) || read != value)
    {
        return std::nullopt;
    }
    return json;
}

QJsonObject writableProperties(const QObject& object, const QList<QMetaProperty>& properties)
{
    QJsonObject json;
    for(const QMetaProperty& property : properties)
    {
        const std::optional<QJsonValue> value = property.isWritable() ? toJson(property.read(&object)) : std::nullopt;
        if(value)
        {
            json.insert(QLatin1StringView(property.name()), *value);
        }
    }
    return json;
}

bool writeProperty(QObject& object, const QList<QMetaProperty>& properties, const QString& name,
                   const QJsonValue& value)
{
    const auto property = std::ranges::find(properties, name, [](const QMetaProperty& known)
                                            { return QString::fromLatin1(known.name()); });
    return property != properties.end() && property->write(&object, value.toVariant());
}

std::optional<ObjectKey> objectKey(const Component& component, const QObject* object)
{
    QHash<QString, qsizetype> counts; // The objects of each name met so far
    const QList<PropertyObject> objects = component.propertyObjects();
    for(const PropertyObject& added : objects)
    {
        if(added.object == object)
        {
            return ObjectKey{.name = added.name, .index = counts.value(added.name)};
        }
        ++counts[added.name];
    }
    return std::nullopt;
}

QObject* object(const Component& component, const ObjectKey& key)
{
    qsizetype index = 0;
    const QList<PropertyObject> objects = component.propertyObjects();
    for(const PropertyObject& added : objects)
    {
        if(added.name == key.name && index++ == key.index)
        {
            return added.object;
        }
    }
    return nullptr;
}

QList<QMetaProperty> savedProperties(const Component& component, const QObject* object)
{
    QList<QMetaProperty> properties;
    if(object == &component)
    {
        if(const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId()))
        {
            properties = info->properties;
        }
    }
    else
    {
        const QList<PropertyObject> objects = component.propertyObjects();
        const auto added = std::ranges::find(objects, object, &PropertyObject::object);
        if(added != objects.end())
        {
            properties = added->properties();
        }
    }
    // Only those a project holds: the others would not be read back.
    properties.removeIf([object](const QMetaProperty& property)
                        { return !property.isWritable() || !toJson(property.read(object)); });
    return properties;
}

QJsonObject values(const Component& component)
{
    QJsonObject json;
    if(const auto* composite = qobject_cast<const Composite*>(&component))
    {
        QJsonArray components;
        for(const std::unique_ptr<Component>& inner : composite->components())
        {
            if(qobject_cast<const Gateway*>(inner.get()) != nullptr)
            {
                continue; // Nothing of its own to set
            }
            QJsonObject innerJson = values(*inner);
            innerJson.insert(u"name", inner->objectName());
            components.append(innerJson);
        }
        json.insert(u"components", components);
        return json;
    }

    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    json.insert(u"properties", writableProperties(component, info ? info->properties : QList<QMetaProperty>()));
    QJsonArray objects;
    const QList<PropertyObject> propertyObjects = component.propertyObjects();
    for(const PropertyObject& added : propertyObjects)
    {
        QJsonObject object;
        object.insert(u"name", added.name);
        object.insert(u"properties", writableProperties(*added.object, added.properties()));
        objects.append(object);
    }
    if(!objects.isEmpty())
    {
        json.insert(u"objects", objects);
    }
    return json;
}

void setValues(Component& component, const QJsonObject& json, const QString& path, QStringList& problems)
{
    if(const auto* composite = qobject_cast<const Composite*>(&component))
    {
        // By name: the structure is the one of its file, which can have changed since they were written.
        const QJsonArray components = json.value(u"components").toArray();
        for(const QJsonValue& value : components)
        {
            const QJsonObject innerJson = value.toObject();
            const QString name = innerJson.value(u"name").toString();
            const auto found =
                std::ranges::find(composite->components(), name,
                                  [](const std::unique_ptr<Component>& inner) { return inner->objectName(); });
            if(found == composite->components().end())
            {
                problems.append(QStringLiteral("%1/%2 is missing: %3 has no component of that name")
                                    .arg(path, name, QFileInfo(composite->filePath()).fileName()));
                continue;
            }
            setValues(**found, innerJson, path + u'/' + name, problems);
        }
        return;
    }

    const std::optional<ComponentInfo> info = ComponentFactory::instance().component(component.componentId());
    const QList<QMetaProperty> componentProperties = info ? info->properties : QList<QMetaProperty>();
    const QJsonObject properties = json.value(u"properties").toObject();
    for(auto property = properties.begin(); property != properties.end(); ++property)
    {
        if(!writeProperty(component, componentProperties, property.key(), property.value()))
        {
            problems.append(QStringLiteral("The property %1 of %2 could not be set").arg(property.key(), path));
        }
    }

    // Each to the first object of its name that none was given yet: those of a name are held in the order they were
    // added.
    QList<PropertyObject> remaining = component.propertyObjects();
    const QJsonArray objects = json.value(u"objects").toArray();
    for(const QJsonValue& objectValue : objects)
    {
        const QJsonObject objectJson = objectValue.toObject();
        const QString objectName = objectJson.value(u"name").toString();
        const auto found = std::ranges::find(remaining, objectName, &PropertyObject::name);
        if(found == remaining.end())
        {
            problems.append(QStringLiteral("The object %1 of %2 is missing").arg(objectName, path));
            continue;
        }
        const PropertyObject restored = *found;
        remaining.erase(found);
        const QJsonObject objectProperties = objectJson.value(u"properties").toObject();
        for(auto property = objectProperties.begin(); property != objectProperties.end(); ++property)
        {
            if(!writeProperty(*restored.object, restored.properties(), property.key(), property.value()))
            {
                problems.append(QStringLiteral("The property %1 of the object %2 of %3 could not be set")
                                    .arg(property.key(), objectName, path));
            }
        }
    }
}

} // namespace AppForge::ProjectJson
