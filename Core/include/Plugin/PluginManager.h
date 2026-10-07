#pragma once

#include <QList>
#include <QString>
#include <map>
#include <memory>
#include <optional>

#include "Core_export.h"
#include "Plugin/PluginInfo.h"

class QPluginLoader;

namespace AppForge
{

// Finds the plugins built with appforge_add_plugin and loads them on demand.
// A loaded plugin is never unloaded: it stays in memory until the application exits.
class CORE_EXPORT PluginManager
{
  public:
    PluginManager();
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // <application directory>/plugins, where appforge_add_plugin writes the plugins; needs a QCoreApplication.
    [[nodiscard]] static QString defaultDirectory();

    // Reads the metadata of the plugins in directory without loading them, and returns the new ones.
    // Skips, with a warning, a file that is not a compatible AppForge plugin or whose id is already known.
    QList<PluginInfo> scan(const QString& directory = defaultDirectory());
    // Reads the metadata of the plugin filePath, anywhere on the disk, without loading it, and returns it; returns it
    // as well when it is already known. nullopt, with errorString() set, when it is not a compatible AppForge plugin
    // or its id is provided by another file.
    std::optional<PluginInfo> addFile(const QString& filePath);

    [[nodiscard]] QList<PluginInfo> plugins() const;
    [[nodiscard]] std::optional<PluginInfo> plugin(const QString& pluginId) const;

    // Loads a scanned plugin, reads its build date and registers its components in ComponentFactory::instance();
    // returns false, with errorString() set, when it is unknown or cannot be loaded.
    bool load(const QString& pluginId);
    [[nodiscard]] bool isLoaded(const QString& pluginId) const;
    [[nodiscard]] QString errorString() const;

  private:
    struct Entry
    {
        PluginInfo info;
        std::unique_ptr<QPluginLoader> loader;
    };

    // The entry of the plugin filePath, canonical; nullptr when it is not known.
    [[nodiscard]] const Entry* findFile(const QString& filePath) const;
    // Adds filePath, canonical and not known yet; nullopt, with error set, when it cannot be used.
    std::optional<PluginInfo> addNewFile(const QString& filePath, QString& error);

    std::map<QString, Entry> m_entries; // By id
    QString m_errorString;
};

} // namespace AppForge
