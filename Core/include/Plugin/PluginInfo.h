#pragma once

#include <QDateTime>
#include <QString>
#include <QVersionNumber>

namespace AppForge
{

// Metadata appforge_add_plugin embeds in a plugin, read without loading it.
struct PluginInfo
{
    QString id;          // <organization>.<project>.<target>
    QString name;        // Target name
    QString description; // Empty when not given
    QVersionNumber version;
    QVersionNumber coreVersion; // Version of Core the plugin was built against
    QDateTime buildDate;        // UTC, refreshed when a source of the plugin changes
    QString filePath;
};

} // namespace AppForge
