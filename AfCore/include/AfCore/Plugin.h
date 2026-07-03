#pragma once

#include <QString>
#include <QDateTime>
#include <QtPlugin>

#include "Version.h"
#include "IPlugin.h"

namespace appforge::core {

class IComponent;

class Plugin : public IPlugin
{
public:
    explicit Plugin(ComponentRegistry& registry
    virtual ~Plugin() = default;

 

    // Hérité via IPlugin
    QString id() const override;

    QString name() const override;

    Version version() const override;

    QDateTime compilationDate() const override;

    QList<IComponent*> components() const override;

    IComponent* buildComponent(const QString& componentId) const override;

    void registerComponents(const IComponent& component) override;

};

}