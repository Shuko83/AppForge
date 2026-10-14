# 🚀 AppForge

**AppForge** is a C++ framework for building modular applications by assembling dynamic components.

It enables developers to construct software systems by combining reusable building blocks (*components*) provided by extensible plugins. Each component exposes configurable properties and communicates with others through explicit connections.

---

## ✨ Features

- 🔌 **Plugin-based architecture**  
  Load and extend functionality dynamically via plugins.

- 🧩 **Component-oriented design**  
  Build applications by assembling independent components.

- ⚙️ **Runtime configuration**  
  Components expose `Q_PROPERTY` for introspection and configuration.

- 🔗 **Flexible connections**  
  Components interact through explicit bindings.

- 💾 **Project serialization**  
  Save and load full application state using `.afp` files.

---

## 🧠 Core Concepts

| Concept     | Description |
|------------|------------|
| Plugin     | Provides component implementations |
| Component  | A reusable functional unit |
| Instance   | A runtime instantiation of a component |
| Binding    | A connection between components |
| Project    | A full application definition |

---

## 🔌 Writing a Plugin

A plugin is declared with `appforge_add_plugin`, available once `Core` is added, or found in an installed AppForge (see [Building a plugin outside AppForge](#building-a-plugin-outside-appforge)). CMake generates its root object and its metadata; the plugin only declares its class, which builds its components (see below).

```cmake
appforge_add_plugin(
    NAME Network
    DESCRIPTION "TCP and UDP components"   # optional
)
```

It accepts every keyword of `cmu_add_target` ([CMakeUtils](CMakeUtils/README.md)) and creates a `MODULE` library `AppForge::<NAME>` linked to `AppForge::Core`, named `<NAME>.afdl` (AppForge DLL) and written to `plugins/` next to the executables (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`, else `<build>/bin`). It is installed in `plugins/` next to each application too (`cmu_plugins_dir` of CMakeUtils); `set(cmu_plugins_dir "")` before the call keeps a plugin out of it. Its metadata is embedded in the binary and read without loading it:

| Key           | Value                                                                        |
| ------------- | ---------------------------------------------------------------------------- |
| `id`          | `<cmu_organization>.<PROJECT_NAME>.<NAME>`, e.g. `FabienDG.AppForge.Network` |
| `name`        | `NAME`                                                                       |
| `description` | `DESCRIPTION`, empty when not given                                          |
| `version`     | `VERSION`, else `PROJECT_VERSION`, else `0.0.0`                              |
| `coreVersion` | Version of `Core` the plugin is built against                                |

Its build date (`__DATE__` and `__TIME__`) is read when it is loaded. `APPFORGE_PLUGIN_ID` holds the id while the plugin compiles.

`AppForge::PluginManager` finds and loads the plugins:

```cpp
AppForge::PluginManager plugins;
for(const AppForge::PluginInfo& info : plugins.scan()) // <application dir>/plugins, nothing is loaded
{
    qInfo() << info.id << info.version << info.description;
}
plugins.load(QStringLiteral("FabienDG.AppForge.Network"));
qInfo() << plugins.plugin(QStringLiteral("FabienDG.AppForge.Network"))->buildDate;
```

`scan()` skips, with a warning, the files that are not AppForge plugins, the plugins built against another major version of `Core` or a newer one, and the ids already found. `addFile(filePath)` adds a single plugin, anywhere on the disk, with the same checks; it tells why a file cannot be added through `errorString()`. Loading a plugin registers its components. `unload(pluginId)` forgets them and destroys its plugin class: none of its components may still be instantiated then, which `ForgeApplication::unloadPlugin()` sees to. A `PluginManager` unloads the plugins it loaded when it is destroyed: it has to outlive their components, as the one of a `ForgeApplication` does. An unloaded plugin can be loaded again, but its library stays in memory until the application exits: its file cannot be replaced meanwhile, and loading it again does not read the file again.

The [exemple](exemple/) folder holds `ExamplePlugin`, a plugin providing the `Greeter` component, and `ExampleApp`, which loads the plugins next to it, lists their components and runs a `Greeter`, whose interface it follows: run it from `<build>/bin` (`<build>/bin/<config>` with Visual Studio).

### Building a plugin outside AppForge

An installed AppForge comes with `appforge_add_plugin`, its templates and the CMakeUtils it relies on: a project of its own only has to find the package.

```cmake
cmake_minimum_required(VERSION 3.19)
project(Acme VERSION 1.0.0 LANGUAGES CXX)

set(cmu_organization "AcmeCorp")   # first part of the plugin ids: AcmeCorp.Acme.Network

find_package(AppForge 0.2 REQUIRED COMPONENTS Core)

add_subdirectory(Network)          # appforge_add_plugin(NAME Network), its sources in Network/src
```

Point `CMAKE_PREFIX_PATH` at the package: `<build>/package`, staged after each build of AppForge, the prefix `cmake --install` wrote it to, or the unzipped package of a [release](https://github.com/Shuko83/AppForge/releases). `find_package` defines `AppForge::Core` and `appforge_add_plugin`, and includes CMakeUtils unless the project already did: call it after `project()`, in the root `CMakeLists.txt`, with the `cmu_*` variables set before it. Qt is found as in any project, through `CMAKE_PREFIX_PATH` or the `QTDIR` environment variable.

The plugin is written to `<build>/bin/plugins` (`<build>/bin/<config>/plugins` with Visual Studio): copy it to `plugins/` next to an application, or open it from Forge with **Open plugin...**. Its `coreVersion` is the version of the installed `Core`, whatever the version of its project. Build it in a configuration AppForge was installed in: Qt does not load a `Debug` plugin into a `Release` application, nor the other way round.

Installed with `-Dcmu_install_mode=PACKAGE`, AppForge is a single prefix whose package has no components: `find_package(AppForge 0.2 REQUIRED)`.

---

## 🧩 Writing a Component

A component belongs to a plugin. It derives from `AppForge::Component` and only exposes what it is, with `Q_PROPERTY`, signals and `Q_CLASSINFO`; it registers itself in the class of its plugin, which builds it. Its logic is in other classes, which the plugin instantiates when it builds it:

```cpp
// Greeter.h
class Greeter : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Greets its recipient when it starts") // optional
    Q_CLASSINFO("category", "Example")                                 // optional
    Q_PROPERTY(QString recipient READ recipient WRITE setRecipient NOTIFY recipientChanged)
    ...
};

// Greeter.cpp
APPFORGE_REGISTER_COMPONENT(ExamplePlugin, Greeter);

// ExamplePlugin.h: the class of the plugin, one per plugin
class ExamplePlugin : public AppForge::Plugin
{
  public:
    void build(Greeter& greeter); // One build() per component registered in the plugin
};

// ExamplePlugin.cpp
APPFORGE_PLUGIN(ExamplePlugin);

void ExamplePlugin::build(Greeter& greeter)
{
    new GreeterLogic(greeter); // A child of greeter: its logic, destroyed with it
}
```

The plugin class is created when its plugin is loaded, after its components are registered in it. Registering a component in a class that does not derive from `AppForge::Plugin`, or has no public `build()` for it, does not compile; a plugin without `APPFORGE_PLUGIN` does not link.

`AppForge::ComponentFactory::instance()` knows the components of the loaded plugins, read from their `QMetaObject` without instantiating them:

| Info          | Value                                                                    |
| ------------- | ------------------------------------------------------------------------ |
| `id`          | `<plugin id>.<name>`, e.g. `FabienDG.AppForge.ExamplePlugin.Greeter`     |
| `name`        | Class name, without its namespace                                        |
| `description` | `Q_CLASSINFO("description", ...)`, empty when not given                  |
| `category`    | `Q_CLASSINFO("category", ...)`, empty when not given                     |
| `properties`  | Its `Q_PROPERTY`, including those of its base classes up to `Component`  |

The factory is the only way to instantiate a component, which it has its plugin build: `new Greeter` does not compile, as a class deriving from `Component` stays abstract until its plugin builds it. A component therefore needs a default constructor and cannot be `final`; registering a class that is not a component, or lacks `Q_OBJECT`, does not compile either.

```cpp
std::unique_ptr<AppForge::Component> greeter =
    AppForge::ComponentFactory::instance().create(QStringLiteral("FabienDG.AppForge.ExamplePlugin.Greeter"));
greeter->setProperty("recipient", QStringLiteral("AppForge")); // Initializing: being configured
greeter->initialize();                                          // Ready
greeter->start();                                               // Running
greeter->stop();                                                // Ready
```

Each step calls a hook the component can override (`onInitialize()`, `onStart()`, `onStop()`); `onInitialize()` and `onStart()` can refuse it by returning `false`. `stateChanged()` is emitted on every change.

A component also provides interfaces: pointers it makes available under a name, such as a `QWidget`, a class of its plugin, or both. It declares each one with `provideInterface<T>(name)` when it is built, so that it is known before the component runs, and calls it again with the pointer once it has one:

```cpp
Greeter::Greeter()
{
    provideInterface<IGreeter>(QStringLiteral("greeter")); // Declared: listed, but unavailable
}

void Greeter::setGreeter(IGreeter* greeter) // Called by its logic
{
    provideInterface<IGreeter>(QStringLiteral("greeter"), greeter); // Available; withdrawn with nullptr
}
```

`provideInterface()` is protected: only the component provides its interfaces. `T` is the type the pointer is retrieved with and is always written: `provideInterface<QWidget>(name, label)` provides a `QLabel` as a `QWidget`. A name keeps the type it was declared with.

The pointer can change while the component lives. Providing another one emits `interfaceUnavailable(name)`, replaces the pointer, then emits `interfaceAvailable(name)`; `interfaceUnavailable()` is also emitted when the interface is withdrawn and when the component is destroyed. Whoever retrieved the pointer stops using it then.

```cpp
for(const AppForge::InterfaceInfo& provided : component->providedInterfaces())
{
    qInfo() << provided.name << provided.type.name() << provided.available; // "greeter" IGreeter* false
}
IGreeter* greeter = component->providedInterface<IGreeter>(QStringLiteral("greeter")); // nullptr while unavailable
```

A component consumes interfaces the same way: `consumeInterface<T>(name, member, onAvailable, onUnavailable)` declares, when it is built, that one of its members is the pointer it gets under a name. The member is `nullptr` until the interface is bound to one another component provides, and that one is available:

```cpp
Window::Window()
{
    consumeInterface<QWidget>(
        QStringLiteral("content"), m_content,  // QWidget* m_content
        [this] { emit contentAvailable(); },   // m_content is set
        [this] { emit contentUnavailable(); }); // m_content is about to be set back to nullptr
}
```

Both callbacks are optional. `onUnavailable` is called while the member still points to what it has to let go of: when the provider withdraws or replaces its pointer, when the interface is unbound, and when one of the two components is destroyed.

`bindInterface()` binds what a component consumes to what another one provides, whatever their names, as long as they have the same type; an interface can be provided to several consumers, and a consumer is bound to one provider at a time:

```cpp
window->bindInterface(QStringLiteral("content"), *ticker, QStringLiteral("display")); // false when it cannot be
window->unbindInterface(QStringLiteral("content"));
```

`consumedInterfaces()` lists what a component consumes, as `providedInterfaces()` does, `binding(name)` tells what one of them is bound to, the provider and the name it provides it under, and `interfacesChanged()` is emitted whenever an interface is declared, or becomes available or unavailable, on either side.

A component can also answer for the properties of the objects it is made of, a widget of its logic, say, with `addPropertiesForQObject()`, which only the component calls, as it does for its interfaces:

```cpp
void Ticker::setDisplay(QWidget* display) // Called by TickerLogic, which owns the display
{
    provideInterface<QWidget>(QStringLiteral("display"), display);
    addPropertiesForQObject(display, QStringLiteral("Display"));
}
```

Forge then edits the properties the object declares with `Q_PROPERTY`, base classes included, under those of the component, and a project holds their values with those of the component. Not `objectName`, nor the properties declared `DESIGNABLE false` or `STORED false`, as the visibility of a widget is. They are edited in a group titled with the name, shared by the objects added with the same one, or right after the properties of the component when no name is given. `propertyObjects()` lists the objects added, each for as long as it lives, and `propertyObjectsChanged()` is emitted when one is added or destroyed. A component adds its objects when it is built: a project sets the properties of the objects it finds when it is opened.

---

## 🔨 Forge

`Forge` is the application that assembles applications from the components of the plugins, live.

It starts with a splash screen, in the colors of the system: the version of AppForge, the Qt and the compiler Forge was built with and when, its copyright and its license. Its main window takes its place after two seconds, or as soon as it is clicked.

For now, it lists:

- the plugins of `plugins/` next to it, with a viewer of the selected one: its metadata, its state, a **Load** button and an **Unload** one; **Open plugin...** loads a plugin from anywhere on the disk. Unloading a plugin takes its components off the list of components. When instances of them are in the edition zone, Forge names them and asks first: they are removed with the plugin, with their links, or nothing is done. A plugin is not unloaded while the application runs: **Unload** is disabled until it is stopped. An unloaded plugin is loaded again with **Load**, or by opening a project that needs it; its file stays in use until Forge is closed, so a plugin built again is only read by the next Forge;
- the components of the loaded plugins, with a viewer of the selected one: what it is and its properties;
- between them, the edition zone of the application: a component dragged from its list and dropped there is instantiated, named after its component and a number (`Greeter1`, `Greeter2`...), and shown where it was dropped. Its instances can be moved and selected. The interfaces of an instance are listed under its names, each with a dot on an edge: those it consumes on the left, a circle; those it provides on the right, a disc while the interface is available and a dotted circle otherwise. Hovering a dot tells at once, in a tooltip, whether the interface is provided or consumed, its type and its state; it does too while a link is being drawn. Dragging from the dot of a provided interface to the dot of a consumed one of the same type, or the other way round, binds them and draws a link between them, in place of the link the consumed one had. While the link is being drawn, the dots it cannot be bound to are dimmed.
- an instance is renamed and colored there too. Double-clicking its names, or **Rename component** in its context menu, edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. An instance keeps a name no other one has, the one a project and `ForgeApplication::component()` know it by: a name already taken is refused, which the status bar tells, and so is no name at all. **Change component color...**, in the same menu, colors its names, which tells it from the others at a glance: the color is chosen with its opacity, and lets the instance show through as much as it is translucent. **Reset component color** gives it back the colors of the style.
- an instance is removed there as well: **Remove component**, in its context menu, removes the one under the cursor from the application, and the Delete key removes the selected ones, all of them when several are; while a name is edited, the key is for what is typed. Its links go with it: what consumed its interfaces is no longer bound to anything. An instance is removed at once, without a question, even while the application runs: it is stopped first.
- in the edition zone too, colored zones that group instances, by what they are for, say: **Add zone**, in its context menu, adds one where it was right-clicked, under a title typed at once. Dragging a zone by its title moves it with what is in it then: the instances whose center is in it, and the zones that are entirely in it. Dragging an edge or a corner resizes it. Double-clicking its title edits it, and right-clicking anywhere in a zone renames it, changes its color or removes it, which leaves what is in it. Between its title and its edges, a zone lets the mouse through: instances are selected there as anywhere else.
- under the edition zone, the editable properties of the instance selected in it, with their values: those that can be written and whose type has an editor (texts, numbers, booleans, dates). They are edited there and follow the changes the component notifies; the others are only listed in the viewer of the component. The properties of the objects the component added with `addPropertiesForQObject()` are edited there as well, after its own, those of a named object in a group under its name: selecting a `Ticker` shows its `interval`, then the properties of its display under **Display**.
- in its **Run** menu, **Start** (F5) and **Stop** (Shift+F5): the application assembled in the edition zone runs there as it does on its own, see Running an Application below. A mark in the corner of each instance tells its state: none before it is initialized, a square once it is, a triangle while it runs. The application can still be edited while it runs; an instance dropped meanwhile waits for the next **Start**. Opening a project, or **New**, stops it.

`ExternalPlugin`, opened with **Open plugin...** from `<build>/external-plugins`, has a pair to bind: the `display` a `Ticker` provides and the `content` a `Window` consumes, which it shows in a window.

It is built in `<build>/bin` (`<build>/bin/<config>` with Visual Studio), next to the plugins.

---

## 📁 Project File

Forge saves what is in its edition zone to a `.afp` (AppForge Project) file, and opens it again, from its **File** menu: **New**, **Open...**, **Save** and **Save As...**. Unless another directory is chosen, the projects are saved to, and looked for in, `scene/` next to Forge, created the first time one is saved: `<build>/bin/scene` (`<build>/bin/<config>/scene` with Visual Studio). A project that has a file is saved and looked for next to it instead. A project file given on its command line is opened as soon as its main window shows: `Forge my.afp`. Before the application is replaced, or Forge closed, it asks whether to save it when it changed since it was saved or opened.

A project file is a JSON object:

```json
{
    "version": 1,
    "plugins": [
        { "id": "FabienDG.AppForge.ExternalPlugin", "file": "../../external-plugins/Debug/ExternalPlugin.afdl" }
    ],
    "components": [
        {
            "name": "Ticker1",
            "type": "FabienDG.AppForge.ExternalPlugin.Ticker",
            "x": 200,
            "y": 120,
            "color": "#804e79a7",
            "properties": { "interval": 250 },
            "objects": [
                { "name": "Display", "properties": { "digitCount": 8, "windowTitle": "Clock" } }
            ]
        },
        { "name": "Window1", "type": "FabienDG.AppForge.ExternalPlugin.Window", "x": 500, "y": 160, "properties": {} }
    ],
    "bindings": [
        { "consumer": "Window1", "consumed": "content", "provider": "Ticker1", "provided": "display" }
    ],
    "zones": [
        { "title": "Display", "color": "#59a14f", "x": 100, "y": 60, "width": 560, "height": 240 }
    ]
}
```

| Key          | Value                                                                                                   |
| ------------ | ------------------------------------------------------------------------------------------------------- |
| `version`    | `1`: a file of another version is not opened                                                            |
| `plugins`    | The plugins of its components, loaded when it is opened; `file` is where a plugin is looked for when it is not found by its `id`, as those of `plugins/` are, relative to the plugin root |
| `components` | The instances: their name, the id of their component, where they are in the edition zone, their color when they were given one, their properties and those of their objects |
| `bindings`   | What a component consumes (`consumer`, `consumed`) bound to what another provides (`provider`, `provided`) |
| `zones`      | The colored zones of the edition zone                                                                   |

`x`, `y`, `color` and `zones` are only for Forge: an application runs without them. The `color` of a component is written `#aarrggbb`, its opacity first: `#804e79a7` is half translucent; `#rrggbb` is read too, as an opaque color.

The plugin root is the directory of Forge unless another one is given on its command line: `Forge --plugin-root <directory> my.afp`; it is the directory of the application that runs a project unless `ForgeApplication::setPluginRoot()` sets it. The files of the plugins are written relative to it, so that a project is opened again wherever Forge and its plugins are moved to together, or wherever the plugins alone are, given their root. A file on another drive than the root cannot be relative to it and is written whole; a file written whole is read as it is.

The `properties` of a component are those that can be written and whose value JSON holds as it is: numbers, booleans and texts, and whatever is read back from its text, as dates are.

Its `objects`, written when it has some, are those it added with `addPropertiesForQObject()`, in the order it added them: the `name` each was added with, empty when it has none, and its `properties`, chosen as those of the component are. When the project is opened, each is given to the first object of that name not given one yet; an object the component no longer has is reported as missing.

Opening a project tells what could not be restored, a plugin that is no longer where it was, say, and opens the rest. The file is then no longer the one **Save** writes to, which would write over what is missing.

---

## ▶️ Running an Application

A project is an application: `AppForge::ForgeApplication` instantiates its components as the project defines them and runs them. This is what Forge does when it starts one, and all the `main` of an application has to do:

```cpp
#include <QApplication>

#include "AppForge/Application/ForgeApplication.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv); // A QCoreApplication is enough when no component shows a widget

    AppForge::ForgeApplication application;
    if(!application.open(QStringLiteral("scene/my.afp")) || !application.start())
    {
        qCritical().noquote() << application.errorString();
        return 1;
    }
    return QApplication::exec(); // The application is stopped when it is destroyed
}
```

It links to `AppForge::Core` only: the plugins of the project are loaded when it is opened, from `plugins/` next to the application or from the file the project gives, relative to the plugin root.

| Function                | What it does                                                                                              |
| ----------------------- | --------------------------------------------------------------------------------------------------------- |
| `open(filePath)`        | Loads the plugins of the project, instantiates its components, sets their properties and binds their interfaces. `false`, with `errorString()` set, when the file cannot be read or anything could not be restored |
| `start()`               | Initializes the components that are not yet, all of them first, then starts them, in the order of the project. `false`, with `errorString()` set, when one refuses: those it started are stopped again |
| `stop()`                | Stops the components that run, the last one first. They can be started again                               |
| `isRunning()`           | Whether it was started and not stopped since; `runningChanged()` is emitted when it changes               |
| `components()`, `component(name)` | Its instances, to reach one from the `main`: `application.component(QStringLiteral("Greeter1"))` |
| `setPluginRoot(directory)` | The directory the files of the plugins are relative to in a project; the directory of the application by default |

An application can also be assembled without a project: `instantiate(componentId)` creates a component in it, `rename(component, name)` names it, unless the name is empty or the one of another instance, `Component::bindInterface()` binds it, `remove(component)` stops it and destroys it, which unbinds what was bound to its interfaces, `unloadPlugin(pluginId)` removes the components of a plugin, those `componentsOf(pluginId)` lists, then unloads it, unless the application runs, and `save()` returns the project of what was assembled. `read()` and `load()` are the two halves of `open()`, for who needs the project itself, as Forge does for what it adds to it.

The [exemple](exemple/) folder holds `ExampleForgeApp`, which runs the project given on its command line until its last window is closed: assemble a `Ticker` and a `Window` in Forge, save them as `scene/clock.afp`, then run `ExampleForgeApp scene/clock.afp` from `<build>/bin` (`<build>/bin/<config>` with Visual Studio).

---

## 🤝 Contributing

The coding conventions the code follows, from formatting and naming to commits and pull requests, are described in [CONVENTIONS.md](CONVENTIONS.md) (in French).

---

## 📄 License

AppForge is licensed under the [Mozilla Public License 2.0](LICENSE). Copyright (C) 2026 Fabien Di Grande.

- Anyone may use, modify and distribute AppForge, in a product of any kind, commercial or not.
- A plugin or an application built on AppForge keeps the license its author chooses, open or closed.
- A file of AppForge stays under the MPL, modified or not: whoever distributes AppForge, as source or as binaries, says where the source of these files is found, with the changes made to them.
- The header `appforge_add_plugin()` generates for a plugin comes from a file of AppForge and stays under the MPL too; the sources of the plugin do not.

This summary does not replace the [license](LICENSE). Each source file carries a two-line notice (copyright and `SPDX-License-Identifier: MPL-2.0`); for the files that cannot, such as `plugin.json.in`, the notice is this one:

> This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

The license covers the files of this repository: Qt and the `CMakeUtils` submodule are separate works, under their own terms.
