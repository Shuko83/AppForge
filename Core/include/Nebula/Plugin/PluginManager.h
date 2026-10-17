// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QString>
#include <map>
#include <memory>
#include <optional>

#include "Core_export.h"
#include "Nebula/Plugin/PluginInfo.h"

class QPluginLoader;

namespace Nebula
{

// Finds the plugins built with nebula_add_plugin, loads them on demand and unloads them. It unloads those it loaded
// when it is destroyed: it has to outlive the components of its plugins.
// The library of a plugin stays in memory once loaded, until the application exits, even unloaded: Qt keeps what it
// learnt from it, the types of its interfaces and of its properties. Unloaded, its file can therefore not be replaced
// and loading it again does not read it again.
class CORE_EXPORT PluginManager
{
  public:
    PluginManager();
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // <application directory>/plugins, where nebula_add_plugin writes the plugins; needs a QCoreApplication.
    [[nodiscard]] static QString defaultDirectory();

    // Reads the metadata of the plugins in directory without loading them, and returns the new ones.
    // Skips, with a warning, a file that is not a compatible Nebula plugin or whose id is already known.
    QList<PluginInfo> scan(const QString& directory = defaultDirectory());
    // Reads the metadata of the plugin filePath, anywhere on the disk, without loading it, and returns it; returns it
    // as well when it is already known. nullopt, with errorString() set, when it is not a compatible Nebula plugin
    // or its id is provided by another file.
    std::optional<PluginInfo> addFile(const QString& filePath);

    [[nodiscard]] QList<PluginInfo> plugins() const;
    [[nodiscard]] std::optional<PluginInfo> plugin(const QString& pluginId) const;

    // Loads a scanned plugin, reads its build date and registers its components in ComponentFactory::instance();
    // returns false, with errorString() set, when it is unknown or cannot be loaded.
    bool load(const QString& pluginId);
    // Unloads a plugin it loaded: its components are no longer registered in ComponentFactory::instance(), and its
    // plugin class is destroyed. None of its components may still be instantiated: see
    // ForgeApplication::unloadPlugin(), which removes them first. It can be loaded again. Returns true when it was not
    // loaded; false, with errorString() set, when it is unknown, and when another manager loaded it, alone or as
    // well: it stays loaded then, for that one.
    bool unload(const QString& pluginId);
    // Whether it is loaded, by any manager.
    [[nodiscard]] bool isLoaded(const QString& pluginId) const;
    [[nodiscard]] QString errorString() const;

  private:
    struct Entry
    {
        PluginInfo info;
        std::unique_ptr<QPluginLoader> loader;
        bool loaded = false; // By this manager, which is then the one to unload it
    };

    // The entry of the plugin filePath, canonical; nullptr when it is not known.
    [[nodiscard]] const Entry* findFile(const QString& filePath) const;
    // Adds filePath, canonical and not known yet; nullopt, with error set, when it cannot be used.
    std::optional<PluginInfo> addNewFile(const QString& filePath, QString& error);

    std::map<QString, Entry> m_entries; // By id
    QString m_errorString;
};

} // namespace Nebula
