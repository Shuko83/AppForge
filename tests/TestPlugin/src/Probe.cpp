#include "Probe.h"

#include "TestPlugin.h"

APPFORGE_REGISTER_COMPONENT(TestPlugin, Probe);

QString Probe::builtBy() const
{
    return m_builtBy;
}

void Probe::setBuiltBy(const QString& builtBy)
{
    m_builtBy = builtBy;
}
