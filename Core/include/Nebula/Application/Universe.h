// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDir>
#include <QJsonObject>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <memory>
#include <optional>
#include <vector>

#include "Core_export.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Plugin/PluginManager.h"
#include "Nebula/Settings/SettingsFile.h"

class QCommandLineParser;

namespace Nebula
{

// An application assembled from the components of the plugins, which it runs. It owns the instances of its components,
// each named, as its objectName, after its component and a number, e.g. Greeter1. It is read from a project file, as
// NebulaStudio writes them, see README.md, or assembled with instantiate() and Component::bindInterface().
// It is either edited or running, as its components are. While it runs, it is not assembled: its components are
// neither instantiated, renamed nor removed. What changes their properties meanwhile, its settings files or its
// components themselves, is undone once it stops: they have the values of the edition again, the ones save() writes.
// Its settings files hold the properties its user changes while it runs, and finds again the next time: see
// SettingsFile.
// It needs a QCoreApplication, and a QApplication as soon as one of its components shows a widget.
class CORE_EXPORT Universe : public QObject
{
    Q_OBJECT

  public:
    // Finds the plugins of PluginManager::defaultDirectory().
    explicit Universe(QObject* parent = nullptr);
    // Stops it.
    ~Universe() override;

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
    // and their "bindings", and its "settings" files when it has some. While it runs, the properties have the values
    // they had when it started, those of the edition.
    [[nodiscard]] QJsonObject save() const;

    // Creates componentId, in the Editing state; nullptr when it is not registered, or while it runs. It is named name,
    // as one read from a project is, unless name is empty or the name of another instance.
    Component* instantiate(const QString& componentId, const QString& name = {});
    // Names instance, one of its components, name: what component() finds it by, and a project holds it as. Returns
    // false, changing nothing, when name is empty or the name of another instance, and while it runs.
    bool rename(Component& instance, const QString& name);
    // Destroys instance, one of its components: nothing may refer to it any longer. What was bound to its interfaces
    // is unbound from them. Returns false, doing nothing, when it is not one of its components, and while it runs.
    bool remove(Component& instance);
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

    // Keeps the values the properties of its components, and of their objects, have: those of the edition. Then gives
    // the properties its settings files hold the values of their files, and starts its components, in the order they
    // were instantiated. Returns false, with errorString() set, when one refuses: those it started are stopped again,
    // and it is edited again, as stop() leaves it. Returns true, doing nothing, while it runs.
    bool start();
    // Stops its components, the last one instantiated first, then its settings files, which write the values of the
    // user, and gives the properties the values of the edition again. Does nothing while it does not run.
    void stop();
    // Whether start() was called with success, and stop() not since.
    [[nodiscard]] bool isRunning() const;

    // Removes its components of the plugin pluginId, as remove() does, then unloads it, see PluginManager::unload().
    // Returns false, with errorString() set, while it runs, nothing being changed then, and when the plugin cannot be
    // unloaded, its components being removed all the same.
    bool unloadPlugin(const QString& pluginId);

    // Why open(), start() or unloadPlugin() returned false.
    [[nodiscard]] QString errorString() const;

  signals:
    void runningChanged(bool running);
    // A component was instantiated, removed, or all of them were: components() no longer holds those removed, which
    // are destroyed once this returns.
    void componentsChanged();
    // A settings file was added or removed, or one of them changed, see SettingsFile::changed().
    void settingsFilesChanged();
    // A settings file could not be read or written, see SettingsFile::failed(); why is a sentence to show.
    void settingsFileFailed(const QString& why);

  private:
    // The properties a project holds of one of its components, or of one of their objects, with the values they had
    // when it started.
    struct EditedValues
    {
        QPointer<QObject> object; // nullptr once it is destroyed
        QList<QMetaProperty> properties;
        QJsonObject values; // By name
    };

    // The properties a project holds of object, among properties: those of the edition while it runs.
    [[nodiscard]] QJsonObject savedValues(const QObject& object, const QList<QMetaProperty>& properties) const;
    // Keeps the values of the edition, see EditedValues.
    void keepEditedValues();
    // Once its components no longer run: stops its settings files, then gives the properties the values of the
    // edition again, and forgets them.
    void returnToEdition();
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
    std::vector<EditedValues> m_editedValues; // While it runs, in the order of the components and of their objects
    QString m_errorString;
};

} // namespace Nebula
