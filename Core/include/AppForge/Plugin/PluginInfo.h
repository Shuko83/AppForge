#pragma once

#include <QDateTime>
#include <QString>
#include <QVersionNumber>

namespace AppForge
{

// Metadata appforge_add_plugin embeds in a plugin, read without loading it, except the build date.
struct PluginInfo
{
    QString id;          // <organization>.<project>.<target>
    QString name;        // Target name
    QString description; // Empty when not given
    QVersionNumber version;
    QVersionNumber coreVersion; // Version of Core the plugin was built against
    QDateTime buildDate;        // Local time of the build machine, known once the plugin is loaded
    QString filePath;
};

} // namespace AppForge
