#pragma once

#include <QString>
#include <QDateTime>
#include <QtPlugin>

#include "Version.h"

namespace appforge::core {

class ComponentRegistry;
class IComponent;

class IPlugin
{
public:
    virtual ~IPlugin() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual Version version() const = 0;
	virtual QDateTime compilationDate() const = 0;

	virtual QList<IComponent*> components() const = 0;
	virtual IComponent* buildComponent(const QString& componentId) const = 0;

	virtual void registerComponents(const IComponent& component) = 0;
};

}

#define APPFORGE_PLUGIN_IID "fdg.appforge.IPlugin/1.0"

Q_DECLARE_INTERFACE(appforge::core::IPlugin, APPFORGE_PLUGIN_IID)
