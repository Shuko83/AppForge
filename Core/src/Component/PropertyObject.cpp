// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Component/PropertyObject.h"

#include <QObject>

namespace Nebula
{

QList<QMetaProperty> PropertyObject::properties() const
{
    QList<QMetaProperty> properties;
    if(object == nullptr)
    {
        return properties;
    }
    const QMetaObject* metaObject = object->metaObject();
    for(int index = QObject::staticMetaObject.propertyCount(); index < metaObject->propertyCount(); ++index)
    {
        const QMetaProperty property = metaObject->property(index);
        if(property.isDesignable() && property.isStored())
        {
            properties.append(property);
        }
    }
    return properties;
}

} // namespace Nebula
