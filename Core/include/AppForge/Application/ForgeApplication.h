// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "AppForge/Component/Component.h"
#include "AppForge/Composite/Composite.h"
#include "AppForge/Plugin/PluginManager.h"
#include "AppForge/Settings/SettingsFile.h"
#include "Core_export.h"

class QCommandLineParser;

namespace AppForge
{

class Assembler;

// An application assembled from the components of the plugins, which it runs. It owns the instances of its components,
// each named, as its objectName, after its component and a number, e.g. Greeter1. It is read from a project file, as
// Forge writes them, see README.md, or assembled with instantiate() and Component::bindInterface().
// Some of its components can be composites, see Composite, each made from a composite file: the application reads each
// of these files once, then places as many instances of it as it needs. The components of a composite are named by
// their path, the name of the composite, a '/', then theirs: Clock1/Ticker1.
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

    // The project the file filePath holds, or the composite: a composite file holds its components as a project does;
    // nullopt, with error set, when it cannot be read or is not of a version this Core reads.
    [[nodiscard]] static std::optional<QJsonObject> read(const QString& filePath, QString& error);
    // Replaces its components with those of project, as save() wrote it: loads its plugins, instantiates its
    // components, places its composites, from their files relative to directory, sets their properties, binds their
    // interfaces and adds its settings files. Returns what could not be restored, a sentence each: none when
    // everything was.
    QStringList load(const QJsonObject& project, const QString& directory = {});
    // load() of the project the file filePath holds, its composite files being relative to its directory. Returns
    // false, with errorString() set, when the file cannot be read, nothing being changed then, and when anything could
    // not be restored.
    bool open(const QString& filePath);
    // Its components as a project: its "version", the "plugins" they are from, the "components" with their values,
    // the file of a composite relative to directory, absolute when it is empty, their "bindings", and its "settings"
    // files when it has some.
    [[nodiscard]] QJsonObject save(const QString& directory = {}) const;

    // Creates componentId, in the Initializing state; nullptr when it is not registered, or is Composite: a composite
    // is made from its file, see instantiateComposite(). It is named name, as one read from a project is, unless name
    // cannot name it, see rename(): it is named after its component then.
    Component* instantiate(const QString& componentId, const QString& name = {});
    // Places an instance of the composite file filePath, in the Initializing state: its components, those of its file
    // with their values, its plugins being loaded when they are not. The file is read the first time one of its
    // instances is placed, then its structure is the one of every instance. It is named name unless name cannot name
    // it, see rename(): it is named after its file then, Clock1 for Clock.afc. Returns nullptr, with errorString() set,
    // when the file cannot be read, is not a composite file, or contains itself, directly or through other composite
    // files. errorString() tells otherwise what of its file could not be restored the first time it is read, a sentence
    // a line: empty when everything was.
    Composite* instantiateComposite(const QString& filePath, const QString& name = {});
    // Names instance, one of its components, name: what component() finds it by, and a project holds it as. Returns
    // false, changing nothing, when name is empty, holds a '/', which separates the names of a path, or is the name of
    // another instance.
    bool rename(Component& instance, const QString& name);
    // Stops instance, one of its components, and destroys it, with its components when it is a composite: nothing may
    // refer to them any longer. What was bound to its interfaces is unbound from them. Does nothing when it is not one
    // of its components.
    void remove(Component& instance);
    // In the order they were instantiated.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components() const;
    // The one path names: one of its components by its name, or one of a composite by its path, the names that lead
    // to it joined with '/', Clock1/Ticker1. nullptr when there is none.
    [[nodiscard]] Component* component(QStringView path) const;
    // The path component() finds component by; empty when it is not one of its components, nor of their composites.
    [[nodiscard]] QString path(const Component& component) const;
    // Those of its components that are of the plugin pluginId, or are composites holding one, in the order they were
    // instantiated.
    [[nodiscard]] QList<Component*> componentsOf(const QString& pluginId) const;
    // Stops it and destroys its components and its settings files: nothing may refer to them any longer. It forgets
    // the composite files it read: they are read again when one of their instances is placed again.
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

    // Removes its components of the plugin pluginId, and its composites that hold one, as remove() does, and forgets
    // the composite files that hold one, then unloads it, see PluginManager::unload(). Returns false, with
    // errorString() set, while it runs, nothing being changed then, and when the plugin cannot be unloaded, its
    // components being removed all the same.
    bool unloadPlugin(const QString& pluginId);

    // Why open(), instantiateComposite(), one of the start() or unloadPlugin() returned false.
    [[nodiscard]] QString errorString() const;

  signals:
    void runningChanged(bool running);
    // A settings file was added or removed, or one of them changed, see SettingsFile::changed().
    void settingsFilesChanged();
    // A settings file could not be read or written, see SettingsFile::failed(); why is a sentence to show.
    void settingsFileFailed(const QString& why);

  private:
    friend class Assembler; // Loads the plugins and reads the composite files

    // A composite file it read, which its instances are made from: a composite that never runs, whose components hold
    // the values they start with, and its bindings, which are not made: bound, a component can act already.
    struct Definition
    {
        std::unique_ptr<Composite> composite;
        QJsonArray bindings;
        bool complete = true; // Whether everything it holds could be restored
    };

    // settings.json, or settings<n>.json with the first number from 2 no settings file is named with.
    [[nodiscard]] QString newSettingsFileName() const;
    void setRunning(bool running);

    PluginManager m_plugins; // Before m_components: it unloads its plugins once their components are destroyed
    QDir m_pluginRoot;
    std::vector<std::unique_ptr<Component>> m_components;
    std::map<QString, Definition> m_definitions;                // By file, canonical: destroyed before m_plugins too
    std::vector<std::unique_ptr<SettingsFile>> m_settingsFiles; // Destroyed first: their entries are of m_components
    bool m_running = false;
    QString m_errorString;
};

} // namespace AppForge
