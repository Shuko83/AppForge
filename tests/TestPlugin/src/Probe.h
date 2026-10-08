#pragma once

#include <QString>

#include "Component/Component.h"

// The component of TestPlugin: tells which plugin class built it.
class Probe : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Component of the PluginManager tests")
    Q_CLASSINFO("category", "Tests")
    Q_PROPERTY(QString builtBy READ builtBy CONSTANT)

  public:
    [[nodiscard]] QString builtBy() const;
    // Called by TestPlugin when it builds the probe, before it is read.
    void setBuiltBy(const QString& builtBy);

  private:
    QString m_builtBy;
};
