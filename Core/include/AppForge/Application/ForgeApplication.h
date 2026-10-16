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
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "AppForge/Component/Component.h"
#include "AppForge/Composite/Composite.h"
#include "AppForge/Composite/Gateway.h"
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
    // In in, one of its composites, rather than in the application when in is given: that changes the structure of the
    // composite file of in, see the structure of a composite below.
    Component* instantiate(const QString& componentId, const QString& name = {}, Composite* in = nullptr);
    // Places an instance of the composite file filePath, in the Initializing state: its components, those of its file
    // with their values, its plugins being loaded when they are not. The file is read the first time one of its
    // instances is placed, see loadComposite(), then its structure is the one of every instance. It is named name
    // unless name cannot name it, see rename(): it is named after its file then, Clock1 for Clock.afc. Returns nullptr,
    // with errorString() set, when the file cannot be read, is not a composite file, or contains itself, directly or
    // through other composite files. errorString() tells otherwise what of its file could not be restored the first
    // time it is read, a sentence a line: empty when everything was.
    // In in, as instantiate() does, unless the composite placed is the one of in, or holds it, which it refuses.
    Composite* instantiateComposite(const QString& filePath, const QString& name = {}, Composite* in = nullptr);
    // Names instance, one of its components, or of one of its composites, name: what component() finds it by, and a
    // project holds it as. Returns false, changing nothing, when name is empty, holds a '/', which separates the names
    // of a path, or is the name of another component of the application, or of the composite instance is one of.
    bool rename(Component& instance, const QString& name);
    // Stops instance, one of its components, or of one of its composites, and destroys it, with its components when it
    // is a composite: nothing may refer to them any longer. What was bound to its interfaces is unbound from them.
    // Does nothing when it is not one of them.
    void remove(Component& instance);
    // Binds consumed, which consumer consumes, to provided, which provider provides, see Component::bindInterface():
    // two components of the application, or of one of its composites. Returns false, changing nothing, when they cannot
    // be bound, or are not of the same composite.
    bool bind(Component& consumer, const QString& consumed, Component& provider, const QString& provided);
    // Adds to the structure of the composite of component a gateway of interfaceName, which component provides or
    // consumes, bound to it: the composite provides what component provides, or consumes what component consumes, under
    // the name of the gateway, see Gateway. It is named name, or after the interface when name cannot name it, see
    // rename(). Returns the one of the composite of component; nullptr, with errorString() set, when component is none
    // of a composite, has no interface of that name, or the composite cannot be changed.
    // A gateway is renamed, which renames the interface of its composite, and removed, which removes it with what is
    // bound to it, as a component is.
    Gateway* expose(Component& component, const QString& interfaceName, const QString& name = {});
    // Has the composite of component expose property, one of component, or of object, one of its objects, or, when
    // component is a composite and object is component, the one it exposes as property: the same property, on the same
    // object, see Composite::exposedProperty(). It is exposed as name, or as the name of the property when name is
    // empty or the name of another property the composite exposes, followed by a number when that one is too. Returns
    // the name it is exposed as; empty, with errorString() set, when component is none of a composite, the property is
    // not one a project holds, the composite exposes it already, or cannot be changed.
    QString exposeProperty(Component& component, QObject* object, const QString& property, const QString& name = {});
    // The name the composite of component exposes property, of object, as exposeProperty() asks about them, as; empty
    // when it does not expose it.
    [[nodiscard]] QString exposedName(const Component& component, const QObject* object, const QString& property) const;
    // composite no longer exposes name, nor do the composites that expose it in turn.
    void unexposeProperty(Composite& composite, const QString& name);
    // composite exposes name as newName, where the composites that expose it in turn find it. Returns false, changing
    // nothing, when newName is empty or the name of another property it exposes, or the composite cannot be changed.
    bool renameExposedProperty(Composite& composite, const QString& name, const QString& newName);
    // In the order they were instantiated.
    [[nodiscard]] const std::vector<std::unique_ptr<Component>>& components() const;
    // The one path names: one of its components by its name, or one of a composite by its path, the names that lead
    // to it joined with '/', Clock1/Ticker1. nullptr when there is none.
    [[nodiscard]] Component* component(QStringView path) const;
    // The path component() finds component by; empty when it is not one of its components, nor of their composites.
    [[nodiscard]] QString path(const Component& component) const;
    // The composite component is one of the components of; nullptr when it is one of the application, or none of them.
    [[nodiscard]] Composite* compositeOf(const Component& component) const;

    // The structure of a composite: what its file holds, its components and their bindings, which all its instances
    // share, and with which it is written. instantiate(), instantiateComposite(), rename(), remove() and bind() change
    // it when they are asked about a component of a composite: every instance of its file changes alike, each keeping
    // the values of its components, and so does what is written to the file, see saveComposite(), whose new components
    // start with their default values. A composite file whose reading met problems is not changed: those calls return
    // false, or nullptr, with errorString() set. A component added to an instance that runs waits until it is started,
    // by start() again or start(instance).

    // Reads the composite file filePath, unless it read it already: its plugins are loaded, but no instance is placed.
    // Returns false, with errorString() set, when it cannot be read, is not a composite file, or contains itself;
    // errorString() tells otherwise what of its file could not be restored the first time it is read, a sentence a
    // line: empty when everything was.
    bool loadComposite(const QString& filePath);
    // The composite files it read, absolute and canonical: those of its composites, of the composites they hold, and
    // those loadComposite() read. Each is read once, until clear() forgets them all.
    [[nodiscard]] QStringList compositeFiles() const;
    // What the composite file filePath, one of compositeFiles(), holds now, as its instances share it: its "version",
    // the "plugins" of its components, its "components", with the values they start with, those of its file for the
    // components it had when it was read, and their "bindings", the files of its composites being relative to its own.
    // Empty when it is none of them.
    [[nodiscard]] QJsonObject saveComposite(const QString& filePath) const;
    // Whether everything the composite file filePath, one of compositeFiles(), holds could be restored when it was
    // read. When it could not, what saveComposite() writes would lose what is missing: it is not changed.
    [[nodiscard]] bool isComplete(const QString& filePath) const;
    // What a composite file of directory holding components, some of the components of the application, and their
    // bindings, would hold, as saveComposite() writes it, their values being the ones they start with. A binding of
    // one of them to one that is not among them is carried by a gateway, named after the interface of theirs, one per
    // interface they provide. nullopt, with errorString() set, when one of them is not one of the application, or is
    // bound to a component of a composite that is not among them.
    [[nodiscard]] std::optional<QJsonObject> compose(const QList<Component*>& components, const QString& directory);
    // Replaces components, as compose() asked about them, with an instance of the composite file filePath, which holds
    // what compose() returned: what they were bound to that is not among them is bound to its gateways, and the
    // properties of theirs that settings files held are then held for its components. Returns nullptr, with
    // errorString() set, when the file cannot be placed, see instantiateComposite(), nothing being changed then.
    Composite* group(const QList<Component*>& components, const QString& filePath);
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
    // again while it runs, it starts those instantiated meanwhile, in it or in its composites that run. Returns false,
    // with errorString() set, when one refuses: those this call started are stopped again, those that ran already go
    // on; when it did not run, its settings files are stopped again too.
    bool start();
    // While it runs, initializes instance, one of its components, or of one of its composites that runs, unless it is
    // already, then starts it unless it runs: one instantiated meanwhile, say. Returns false, with errorString() set,
    // when it does not run, or the composite of instance does not, nothing being started then, when instance refuses,
    // or is none of its components.
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

    // Why open(), one of the calls that change components or composite files, one of the start() or unloadPlugin()
    // returned false, or nullptr.
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

    // An instance of a composite file: bound, as those of the application are, or unbound, as those of the definitions.
    struct Instance
    {
        Composite* composite = nullptr;
        bool bound = true;
    };

    // A binding of a component grouped into a composite to one that is not, which a gateway carries then.
    struct Crossing
    {
        Component* inside = nullptr; // One of those grouped
        QString insideInterface;
        Component* outside = nullptr; // One that is not
        QString outsideInterface;
        bool insideProvides = false; // Whether inside provides what outside consumes, rather than the other way round
        QString gateway;             // Its name: one per interface inside provides
    };

    // The bindings between components, some of the application, and those that are not among them, with the gateways
    // that would carry them; nullopt, with errorString() set, when one of them is bound to a component of a composite
    // that is not among them.
    [[nodiscard]] std::optional<QList<Crossing>> crossings(const QList<Component*>& components);
    // In the definitions that hold instances of the composite file filePath, the bindings to their interface name are
    // to newName, or are no longer there when newName is empty.
    void renameBindings(const QString& filePath, const QString& name, const QString& newName);
    // Calls visit with every composite: those of the application and of its composites, and the definitions with the
    // composites they hold.
    void forEachComposite(const std::function<void(Composite&)>& visit) const;
    // Every instance of the composite file filePath no longer exposes name, nor do, in turn, the composites that
    // exposed it.
    void unexposeEverywhere(const QString& filePath, const QString& name);

    // Every instance of the composite file filePath: those of the application and of its composites, the definition of
    // filePath, and the instances the other definitions hold.
    [[nodiscard]] QList<Instance> instancesOf(const QString& filePath) const;
    // Whether the structure of the composite file of composite can be changed: its reading met no problem. Sets
    // errorString() when it cannot.
    bool canChange(const Composite& composite);
    // Whether the definition of the composite file filePath holds a composite of the file held, at any depth.
    [[nodiscard]] bool holdsFile(const QString& filePath, const QString& held) const;
    // Initializes, then starts, those of components that are not yet, and those added to the composites among them
    // that run, appending to started those it starts. Returns false, with errorString() set, at the first refusal.
    bool startAll(const std::vector<std::unique_ptr<Component>>& components, std::vector<Component*>& started);
    // The path of what refused in refused, which just refused to be initialized or started: the one of refused, or the
    // one of what refused in it when it is a composite.
    [[nodiscard]] QString refusalIn(const Component& refused) const;
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
