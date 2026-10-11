#pragma once

#include <QMetaType>
#include <QString>

namespace AppForge
{

// An interface a component provides: a pointer it makes available under a name, see Component::provideInterface().
struct InterfaceInfo
{
    QString name;           // Unique among the interfaces of its component
    QMetaType type;         // Of the pointer, e.g. QWidget*
    bool available = false; // Whether its pointer can be retrieved, see IComponent::providedInterface()
};

} // namespace AppForge
