#pragma once

#include <QString>

class QObject;

namespace AppForge
{

// An object whose properties are edited with those of its component: see Component::addPropertiesForQObject().
struct PropertyObject
{
    QObject* object = nullptr;
    QString name; // Of the group its properties are edited in; with those of the component when empty
};

} // namespace AppForge
