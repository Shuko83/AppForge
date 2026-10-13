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
#include "Core_export.h"

namespace AppForge
{

// An application assembled from the components of the plugins, which it runs. It owns the instances of its components,
// each named, as its objectName, after its component and a number, e.g. Greeter1. It is read from a project file, as
// Forge writes them, see README.md, or assembled with instantiate() and Component::bindInterface().
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

    // The project the file filePath holds; nullopt, with error set, when it cannot be read or is not a project of the
    // version this Core reads.
    [[nodiscard]] static std::optional<QJsonObject> read(const QString& filePath, QString& error);
    // Replaces its components with those of project, as save() wrote it: loads its plugins, instantiates its
    // components, sets their properties and binds their interfaces. Returns what could not be restored, a sentence
    // each: none when everything was.
    QStringList load(const QJsonObject& project);
    // load() of the project the file filePath holds. Returns false, with errorString() set, when the file cannot be
    // read, nothing being changed then, and when anything could not be restored.
    bool open(const QString& filePath);
    // Its components as a project: its "version", the "plugins" they are from, the "components" with their properties
    // and their "bindings".
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
    // Stops it and destroys its components: nothing may refer to them any longer.
    void clear();

    // Initializes the components that are not yet, then starts those that do not run, in the order they were
    // instantiated. Returns false, with errorString() set, when one refuses: those it started are stopped again.
    bool start();
    // Stops the components that run, the last one instantiated first.
    void stop();
    // Whether it was started, and not stopped since. A component instantiated meanwhile waits for the next start().
    [[nodiscard]] bool isRunning() const;

    // Removes its components of the plugin pluginId, as remove() does, then unloads it, see PluginManager::unload().
    // Returns false, with errorString() set, while it runs, nothing being changed then, and when the plugin cannot be
    // unloaded, its components being removed all the same.
    bool unloadPlugin(const QString& pluginId);

    // Why open(), start() or unloadPlugin() returned false.
    [[nodiscard]] QString errorString() const;

  signals:
    void runningChanged(bool running);

  private:
    // Whether an instance is named name.
    [[nodiscard]] bool contains(const QString& name) const;
    // componentName and the first number no instance is named with.
    [[nodiscard]] QString newName(const QString& componentName) const;
    void setRunning(bool running);

    PluginManager m_plugins; // Before m_components: it unloads its plugins once their components are destroyed
    QDir m_pluginRoot;
    std::vector<std::unique_ptr<Component>> m_components;
    bool m_running = false;
    QString m_errorString;
};

} // namespace AppForge
