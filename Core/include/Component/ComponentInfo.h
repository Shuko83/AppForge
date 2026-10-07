#pragma once

#include <QList>
#include <QMetaProperty>
#include <QString>

namespace AppForge
{

// What ComponentFactory knows about a registered component, read from its QMetaObject without instantiating it.
struct ComponentInfo
{
    QString id;                      // <plugin id>.<name>
    QString name;                    // Class name, without its namespace
    QString description;             // Q_CLASSINFO("description", ...), empty when not given
    QString category;                // Q_CLASSINFO("category", ...), empty when not given
    QString pluginId;                // Plugin that registered it
    QList<QMetaProperty> properties; // Its Q_PROPERTY, not those of QObject and Component
};

} // namespace AppForge
