// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDir>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <optional>
#include <vector>

#include "AppForge/Component/Component.h"
#include "AppForge/Plugin/PluginManager.h"
#include "AppForge/Settings/SettingsFile.h"
#include "Core_export.h"

class QCommandLineParser;

namespace AppForge
{

// An application assembled from the components of the plugins, which it runs. It owns the instances of its components,
// each named, as its objectName, after its component and a number, e.g. Greeter1. It is read from a project file, as
// Forge writes them, see README.md, or assembled with instantiate() and Component::bindInterface().
// Its settings files hold the properties its user changes while it runs, and finds again the next time: see
// SettingsFile.
// It needs a QCoreApplication, and a QApplication as soon as one of its components shows a widget.
class CORE_EXPORT ForgeApplication : public QObject
{
    Q_OBJECT

  public:
    // Finds the plugins of PluginManager::defaultDirectory().
    explicit ForgeApplication(QObject* parent = nullptr);
    // Stops it.
    ~ForgeApplication() override;

    // The plugins its components are from: those it found, and those the projects it loaded name.
    [[nodiscard]] PluginManager& plugins();
    [[nodiscard]] const PluginManager& plugins() const;

    // The directory the files of the plugins are relative to in a project: the one of the application until it is set.
    [[nodiscard]] QString pluginRoot() const;
    void setPluginRoot(const QString& directory);

    // Adds to parser, before it processes the command line, the options every application assembled with it
    // understands: --plugin-dir <directory>, as many times as needed, and --plugin-root <directory>.
    static void addCommandLineOptions(QCommandLineParser& parser);
    // Does what the options addCommandLineOptions() added to parser ask, once it processed the command line: scans
    // each --plugin-dir after plugins/, in their order, skipping with a warning one that is not a directory, then sets
    // the plugin root to --plugin-root when it is given. Before a project is loaded, for its plugins to be found there.
    void applyCommandLine(const QCommandLineParser& parser);

    // The project the file filePath holds; nullopt, with error set, when it cannot be read or is not a project of the
    // version this Core reads.
    [[nodiscard]] static std::optional<QJsonObject> read(const QString& filePath, QString& error);
    // Replaces its components with those of project, as save() wrote it: loads its plugins, instantiates its
    // components, sets their properties, binds their interfaces and adds its settings files. Returns what could not be
    // restored, a sentence each: none when everything was.
    QStringList load(const QJsonObject& project);
    // load() of the project the file filePath holds. Returns false, with errorString() set, when the file cannot be
    // read, nothing being changed then, and when anything could not be restored.
    bool open(const QString& filePath);
    // Its components as a project: its "version", the "plugins" they are from, the "components" with their properties
    // and their "bindings", and its "settings" files when it has some.
    [[nodiscard]] QJsonObject save() const;

    // Creates componentId, in the Initializing state; nullptr when it is not registered. It is named name, as one read
    // from a project is, unless name is empty or the name of another instance.
    Component* instantiate(const QString& componentId, const QString& name = {});
    // Names instance, one of its components, name: what component() finds it by, and a project holds it as. Returns
    // false, changing nothing, when name is empty or the name of another instance.
    bool rename(Component& instance, const QString& name);
    // Stops instance, one of its components, and destroys it: nothing may refer to it any longer. What was bound to
    // its interfaces is unbound from them. Does nothing when it is not one of its components.
    void remove(Component& instance);
    // In the order they were instantiated.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components() const;
    // The one named name; nullptr when none is.
    [[nodiscard]] Component* component(const QString& name) const;
    // Those of the plugin pluginId, in the order they were instantiated.
    [[nodiscard]] QList<Component*> componentsOf(const QString& pluginId) const;
    // Stops it and destroys its components and its settings files: nothing may refer to them any longer.
    void clear();

    // Adds a settings file, which holds no property yet, named settings.json, or settings2.json, settings3.json... when
    // another one is: see SettingsFile.
    SettingsFile& addSettingsFile();
    // Destroys file, one of its settings files, written and stopped first when it runs: its properties have their
    // default values again. Does nothing when it is not one of them.
    void removeSettingsFile(SettingsFile& file);
    // In the order they were added.
    [[nodiscard]] const std::vector<std::unique_ptr<SettingsFile>>& settingsFiles() const;
    // The one of its settings files that holds entry; nullptr when none does.
    [[nodiscard]] SettingsFile* settingsFile(const SettingsEntry& entry) const;
    // The one of its settings files named fileName, whatever its case; nullptr when none is.
    [[nodiscard]] SettingsFile* settingsFile(const QString& fileName) const;

    // Gives the properties its settings files hold their values, unless it runs already, then initializes the
    // components that are not yet, and starts those that do not run, in the order they were instantiated: called
    // again while it runs, it starts those instantiated meanwhile. Returns false, with errorString() set, when one
    // refuses: those this call started are stopped again, those that ran already go on; when it did not run, its
    // settings files are stopped again too.
    bool start();
    // While it runs, initializes instance, one of its components, unless it is already, then starts it unless it
    // runs: one instantiated meanwhile, say. Returns false, with errorString() set, when it does not run, nothing being
    // started then, when instance refuses, or is not one of its components.
    bool start(Component& instance);
    // Stops the components that run, the last one instantiated first, then its settings files: their properties have
    // their default values again.
    void stop();
    // Whether start() was called with success, and stop() not since. A component instantiated meanwhile waits until
    // it is started, by start() again or start(instance).
    [[nodiscard]] bool isRunning() const;

    // Removes its components of the plugin pluginId, as remove() does, then unloads it, see PluginManager::unload().
    // Returns false, with errorString() set, while it runs, nothing being changed then, and when the plugin cannot be
    // unloaded, its components being removed all the same.
    bool unloadPlugin(const QString& pluginId);

    // Why open(), one of the start() or unloadPlugin() returned false.
    [[nodiscard]] QString errorString() const;

  signals:
    void runningChanged(bool running);
    // A settings file was added or removed, or one of them changed, see SettingsFile::changed().
    void settingsFilesChanged();
    // A settings file could not be read or written, see SettingsFile::failed(); why is a sentence to show.
    void settingsFileFailed(const QString& why);

  private:
    // Whether an instance is named name.
    [[nodiscard]] bool contains(const QString& name) const;
    // componentName and the first number no instance is named with.
    [[nodiscard]] QString newName(const QString& componentName) const;
    // settings.json, or settings<n>.json with the first number from 2 no settings file is named with.
    [[nodiscard]] QString newSettingsFileName() const;
    void setRunning(bool running);

    PluginManager m_plugins; // Before m_components: it unloads its plugins once their components are destroyed
    QDir m_pluginRoot;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<std::unique_ptr<SettingsFile>> m_settingsFiles; // Destroyed first: their entries are of m_components
    bool m_running = false;
    QString m_errorString;
};

} // namespace AppForge
