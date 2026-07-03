#pragma once

#include <QString>
#include <QDateTime>
#include <QtPlugin>

#include "afCore/Version.h"
#include "afCore/Plugin.h"

namespace appforge::core {

QString appforge::core::Plugin::id() const
{
    return QString();
}

QString Plugin::name() const
{
    return QString();
}

Version Plugin::version() const
{
    return Version();
}

QDateTime Plugin::compilationDate() const
{
    return QDateTime();
}

QList<IComponent*> Plugin::components() const
{
    return QList<IComponent*>();
}

IComponent* Plugin::buildComponent(const QString& componentId) const
{
    return nullptr;
}

void Plugin::registerComponents(const IComponent& component)
{
}

}
