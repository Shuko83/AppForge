#include <QCoreApplication>
#include <QTextStream>
#include <optional>

#include "Plugin/PluginManager.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    // scan() reads the metadata without loading the plugins; load() is needed for the build date.
    AppForge::PluginManager plugins;
    const QList<AppForge::PluginInfo> found = plugins.scan();
    out << found.size() << " plugin(s) in " << AppForge::PluginManager::defaultDirectory() << '\n';
    for(const AppForge::PluginInfo& info : found)
    {
        out << '\n' << info.id << '\n';
        out << "  name:        " << info.name << '\n';
        out << "  description: " << info.description << '\n';
        out << "  version:     " << info.version.toString() << " (Core " << info.coreVersion.toString() << ")\n";
        out << "  file:        " << info.filePath << '\n';
        if(!plugins.load(info.id))
        {
            out << "  not loaded:  " << plugins.errorString() << '\n';
            continue;
        }
        if(const std::optional<AppForge::PluginInfo> loaded = plugins.plugin(info.id))
        {
            out << "  build date:  " << loaded->buildDate.toString(Qt::ISODate) << '\n';
        }
    }
    return 0;
}
