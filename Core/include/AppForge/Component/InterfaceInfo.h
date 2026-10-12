#pragma once

#include <QMetaType>
#include <QString>

namespace AppForge
{

// An interface of a component: a pointer it provides under a name, see Component::provideInterface(), or one it
// consumes, see Component::consumeInterface().
struct InterfaceInfo
{
    QString name;           // Unique among the interfaces its component provides, or among those it consumes
    QMetaType type;         // Of the pointer, e.g. QWidget*
    bool available = false; // Whether the component has the pointer: it provides one, or the one it consumes is set
};

class Component;

// What an interface a component consumes is bound to: see Component::bindInterface().
struct InterfaceBinding
{
    Component* provider = nullptr; // nullptr when it is bound to nothing
    QString providedName;          // What provider provides it as
};

} // namespace AppForge
