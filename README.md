# 🚀 Nebula

<img src="NebulaStudio/resources/nebula.svg" width="96" alt="The logo of Nebula">

**Nebula** is a C++ framework for building modular applications by assembling dynamic components.

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
  Save and load full application state using `.nbp` files.

- 🗂️ **User settings**  
  Keep the values the user of an application gives some properties, from one run to the next, in settings files.

---

## 🧠 Core Concepts

| Concept     | Description |
|------------|------------|
| Plugin     | Provides component implementations |
| Component  | A reusable functional unit |
| Instance   | A runtime instantiation of a component |
| Binding    | A connection between components |
| Project    | A full application definition |
| Settings file | The properties of instances whose values the user keeps from one run to the next |

The common vocabulary of the framework, Nebula, Univers, Galaxy, Astre and Comet, is defined in the [glossary](docs/glossary.md), with the name each of these notions has in the code today.

---

## 🔌 Writing a Plugin

A plugin is declared with `nebula_add_plugin`, available once `Core` is added, or found in an installed Nebula (see [Building a plugin outside Nebula](#building-a-plugin-outside-nebula)). CMake generates its root object and its metadata; the plugin only declares its class, which builds its components (see below).

```cmake
nebula_add_plugin(
    NAME Network
    DESCRIPTION "TCP and UDP components"   # optional
)
```

It accepts every keyword of `cmu_add_target` ([CMakeUtils](CMakeUtils/README.md)) and creates a `MODULE` library `Nebula::<NAME>` linked to `Nebula::Core`, named `<NAME>.nbdl` (Nebula DLL) and written to `plugins/` next to the executables (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`, else `<build>/bin`). It is installed in `plugins/` next to each application too (`cmu_plugins_dir` of CMakeUtils); `set(cmu_plugins_dir "")` before the call keeps a plugin out of it. Its metadata is embedded in the binary and read without loading it:

| Key           | Value                                                                        |
| ------------- | ---------------------------------------------------------------------------- |
| `id`          | `<cmu_organization>.<PROJECT_NAME>.<NAME>`, e.g. `FabienDG.Nebula.Network` |
| `name`        | `NAME`                                                                       |
| `description` | `DESCRIPTION`, empty when not given                                          |
| `version`     | `VERSION`, else `PROJECT_VERSION`, else `0.0.0`                              |
| `coreVersion` | Version of `Core` the plugin is built against                                |

Its build date (`__DATE__` and `__TIME__`) is read when it is loaded. `NEBULA_PLUGIN_ID` holds the id while the plugin compiles.

`Nebula::PluginManager` finds and loads the plugins:

```cpp
Nebula::PluginManager plugins;
for(const Nebula::PluginInfo& info : plugins.scan()) // <application dir>/plugins, nothing is loaded
{
    qInfo() << info.id << info.version << info.description;
}
plugins.load(QStringLiteral("FabienDG.Nebula.Network"));
qInfo() << plugins.plugin(QStringLiteral("FabienDG.Nebula.Network"))->buildDate;
```

`scan()` skips, with a warning, the files that are not Nebula plugins, the plugins built against another major version of `Core` or a newer one, and the ids already found. `addFile(filePath)` adds a single plugin, anywhere on the disk, with the same checks; it tells why a file cannot be added through `errorString()`. Loading a plugin registers its components. `unload(pluginId)` forgets them and destroys its plugin class: none of its components may still be instantiated then, which `Universe::unloadPlugin()` sees to. A `PluginManager` unloads the plugins it loaded when it is destroyed: it has to outlive their components, as the one of a `Universe` does. An unloaded plugin can be loaded again, but its library stays in memory until the application exits: its file cannot be replaced meanwhile, and loading it again does not read the file again.

The [exemple](exemple/) folder holds `ExamplePlugin`, a plugin providing the `Greeter` component, and `ExampleApp`, which loads the plugins next to it, lists their components and runs a `Greeter`, whose interface it follows: run it from `<build>/bin` (`<build>/bin/<config>` with Visual Studio).

### Building a plugin outside Nebula

An installed Nebula comes with `nebula_add_plugin`, its templates and the CMakeUtils it relies on: a project of its own only has to find the package.

```cmake
cmake_minimum_required(VERSION 3.19)
project(Acme VERSION 1.0.0 LANGUAGES CXX)

set(cmu_organization "AcmeCorp")   # first part of the plugin ids: AcmeCorp.Acme.Network

find_package(Nebula 0.3 REQUIRED COMPONENTS Core)

add_subdirectory(Network)          # nebula_add_plugin(NAME Network), its sources in Network/src
```

Point `CMAKE_PREFIX_PATH` at the package: `<build>/package`, staged after each build of Nebula, the prefix `cmake --install` wrote it to, or the unzipped package of a [release](https://github.com/Shuko83/AppForge/releases). `find_package` defines `Nebula::Core` and `nebula_add_plugin`, and includes CMakeUtils unless the project already did: call it after `project()`, in the root `CMakeLists.txt`, with the `cmu_*` variables set before it. Qt is found as in any project, through `CMAKE_PREFIX_PATH` or the `QTDIR` environment variable.

The plugin is written to `<build>/bin/plugins` (`<build>/bin/<config>/plugins` with Visual Studio): copy it to `plugins/` next to an application, or open it from NebulaStudio with **Open plugin...**. Its `coreVersion` is the version of the installed `Core`, whatever the version of its project. Build it in a configuration Nebula was installed in: Qt does not load a `Debug` plugin into a `Release` application, nor the other way round.

Installed with `-Dcmu_install_mode=PACKAGE`, Nebula is a single prefix whose package has no components: `find_package(Nebula 0.3 REQUIRED)`.

---

## 🧩 Writing a Component

A component belongs to a plugin. It derives from `Nebula::Component` and only exposes what it is, with `Q_PROPERTY`, signals and `Q_CLASSINFO`; it registers itself in the class of its plugin, which builds it. Its logic is in other classes, which the plugin instantiates when it builds it:

```cpp
// Greeter.h
class Greeter : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Greets its recipient when it starts") // optional
    Q_CLASSINFO("category", "Example")                                 // optional
    Q_PROPERTY(QString recipient READ recipient WRITE setRecipient NOTIFY recipientChanged)
    ...
};

// Greeter.cpp
NEBULA_REGISTER_COMPONENT(ExamplePlugin, Greeter);

// ExamplePlugin.h: the class of the plugin, one per plugin
class ExamplePlugin : public Nebula::Plugin
{
  public:
    void build(Greeter& greeter); // One build() per component registered in the plugin
};

// ExamplePlugin.cpp
NEBULA_PLUGIN(ExamplePlugin);

void ExamplePlugin::build(Greeter& greeter)
{
    new GreeterLogic(greeter); // A child of greeter: its logic, destroyed with it
}
```

The plugin class is created when its plugin is loaded, after its components are registered in it. Registering a component in a class that does not derive from `Nebula::Plugin`, or has no public `build()` for it, does not compile; a plugin without `NEBULA_PLUGIN` does not link.

`Nebula::ComponentFactory::instance()` knows the components of the loaded plugins, read from their `QMetaObject` without instantiating them:

| Info          | Value                                                                    |
| ------------- | ------------------------------------------------------------------------ |
| `id`          | `<plugin id>.<name>`, e.g. `FabienDG.Nebula.ExamplePlugin.Greeter`     |
| `name`        | Class name, without its namespace                                        |
| `description` | `Q_CLASSINFO("description", ...)`, empty when not given                  |
| `category`    | `Q_CLASSINFO("category", ...)`, empty when not given                     |
| `properties`  | Its `Q_PROPERTY`, including those of its base classes up to `Component`  |

The factory is the only way to instantiate a component, which it has its plugin build: `new Greeter` does not compile, as a class deriving from `Component` stays abstract until its plugin builds it. A component therefore needs a default constructor and cannot be `final`; registering a class that is not a component, or lacks `Q_OBJECT`, does not compile either.

```cpp
std::unique_ptr<Nebula::Component> greeter =
    Nebula::ComponentFactory::instance().create(QStringLiteral("FabienDG.Nebula.ExamplePlugin.Greeter"));
greeter->setProperty("recipient", QStringLiteral("Nebula")); // Editing: being configured
greeter->start();                                               // Running
greeter->stop();                                                // Editing
```

A component has two modes: `Editing`, in which it is created, its properties set and its interfaces bound, and `Running`. Each step calls a hook the component can override (`onStart()`, `onStop()`); `onStart()` can refuse it by returning `false`. `stateChanged()` is emitted on every change.

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
for(const Nebula::InterfaceInfo& provided : component->providedInterfaces())
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

A component that does not work without an interface it consumes says so with `Nebula::InterfaceRequirement::Mandatory`, after the member; interfaces are `Optional` otherwise. It is only told to who assembles components, by the `requirement` of `consumedInterfaces()`, and NebulaStudio marks the interface with an asterisk: nothing checks it, and the component starts without it all the same.

```cpp
consumeInterface<QWidget>(
    QStringLiteral("content"), m_content, Nebula::InterfaceRequirement::Mandatory,
    [this] { emit contentAvailable(); }, [this] { emit contentUnavailable(); });
```

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

NebulaStudio then edits the properties the object declares with `Q_PROPERTY`, base classes included, under those of the component, and a project holds their values with those of the component. Not `objectName`, nor the properties declared `DESIGNABLE false` or `STORED false`, as the visibility of a widget is. They are edited in a group titled with the name, shared by the objects added with the same one, or right after the properties of the component when no name is given. `propertyObjects()` lists the objects added, each for as long as it lives, and `propertyObjectsChanged()` is emitted when one is added or destroyed. A component adds its objects when it is built: a project sets the properties of the objects it finds when it is opened.

---

## 🔨 NebulaStudio

`NebulaStudio` is the application that assembles applications from the components of the plugins, live.

It starts with a splash screen, in the colors of its theme: the version of Nebula, the Qt and the compiler NebulaStudio was built with and when, its copyright and its license. Its main window takes its place after two seconds, or as soon as it is clicked.

For now, it lists:

- the plugins of `plugins/` next to it, and those of the directories given on its command line, each with `--plugin-dir`: `NebulaStudio --plugin-dir <directory> --plugin-dir <another one>`, scanned after `plugins/`, in their order, a plugin found again being skipped. A viewer shows the selected one: its metadata, its state, a **Load** button and an **Unload** one; **Open plugin...** loads a plugin from anywhere on the disk. Unloading a plugin takes its components off the list of components. When instances of them are in the edition zone, NebulaStudio names them and asks first: they are removed with the plugin, with their links, or nothing is done. A plugin is not unloaded while the application runs: **Unload** is disabled until it is stopped. An unloaded plugin is loaded again with **Load**, or by opening a project that needs it; its file stays in use until NebulaStudio is closed, so a plugin built again is only read by the next NebulaStudio;
- the components of the loaded plugins, with a viewer of the selected one: what it is and its properties. The search bar above them, also reached with Ctrl+F, keeps those whose name holds what is typed, whatever its case: `lic` finds `maLicence`; next to it, **Description** or **Category** searches their descriptions or their categories instead. When the selected component is no longer listed, another one is selected, or none;
- between them, the edition zone of the application: a component dragged from its list and dropped there is instantiated, named after its component and a number (`Greeter1`, `Greeter2`...), and shown where it was dropped. Its instances can be moved and selected. The interfaces of an instance are listed under its names, each with a dot on an edge: those it consumes on the left, a circle, with an asterisk after the name of those the component cannot do without; those it provides on the right, a disc while the interface is available and a dotted circle otherwise. Hovering a dot tells at once, in a tooltip, whether the interface is provided or consumed, its type, whether it is mandatory and its state; it does too while a link is being drawn. Dragging from the dot of a provided interface to the dot of a consumed one of the same type, or the other way round, binds them and draws a link between them, in place of the link the consumed one had. While the link is being drawn, the dots it cannot be bound to are dimmed.
- an instance is renamed and colored there too. Double-clicking its names, or **Rename component** in its context menu, edits its name in place, until Enter is pressed or anything else is clicked; Escape gives the edition up. An instance keeps a name no other one has, the one a project and `Universe::component()` know it by: a name already taken is refused, which the status bar tells, and so is no name at all. **Change component color...**, in the same menu, colors its names, which tells it from the others at a glance: the color is chosen with its opacity, and lets the instance show through as much as it is translucent. **Reset component color** gives it back the colors of the style.
- an instance is removed there as well: **Remove component**, in its context menu, removes the one under the cursor from the application, and the Delete key removes the selected ones, all of them when several are; while a name is edited, the key is for what is typed. Its links go with it: what consumed its interfaces is no longer bound to anything. An instance is removed at once, without a question.
- in the edition zone too, colored zones that group instances, by what they are for, say: **Add zone**, in its context menu, adds one where it was right-clicked, under a title typed at once; the menu of an instance does not offer it. Dragging a zone by its title moves it with what is in it then: the instances whose center is in it, and the zones that are entirely in it. Dragging an edge or a corner resizes it. Double-clicking its title edits it, and right-clicking anywhere in a zone renames it, changes its color or removes it, which leaves what is in it. Between its title and its edges, a zone lets the mouse through: instances are selected there as anywhere else.
- in the edition zone as well, the settings files of the application, which keep the values its user gives some properties, the profile they chose, say, from one run to the next; the value the project gives such a property becomes its default value. **Add settings file**, in the context menu of the edition zone, adds one where it was right-clicked, a sheet titled with the name of its file, and opens its dialog at once: the name of its file, `settings.json` unless another one is given, or `settings2.json`, `settings3.json`... when another file has that name, its directory, the configuration directory of the user unless another one is given, then the properties it holds, checked among those of the instances, and of their objects, that a project saves. A property is held by a single file: one another file holds is listed with the name of that file, and cannot be checked. Several instances can keep their properties in the same file, and an application can have several files, each of its own name: the dialog is not accepted with the name of another file, whatever its case and their directories, as the two would be written over each other. Dashed lines join a file to the instances whose properties it holds, and its tooltip tells where it is written and what it holds. Double-clicking it, or **Edit settings file...** in its context menu, opens its dialog again; **Write default values** writes the values of the project to the file, in place of those of the user, to start trials over or to make the file to deliver; **Remove settings file**, or the Delete key, removes it. Removing an instance removes its properties from the files; renaming it keeps them, but the values its user gave it stay under its former name in the file.
- under the edition zone, the editable properties of the instance selected in it, with their values: those that can be written and whose type has an editor (texts, numbers, booleans, dates). They are edited there and follow the changes the component notifies; the others are only listed in the viewer of the component. The properties of the objects the component added with `addPropertiesForQObject()` are edited there as well, after its own, those of a named object in a group under its name: selecting a `Ticker` shows its `interval`, then the properties of its display under **Display**. The name of a property a settings file holds is in italics: its value is the default one. While the application runs, they are only shown.
- in its **Run** menu, **Start** (F5) and **Stop** (Shift+F5): the application assembled in the edition zone runs there as it does on its own, see Running an Application below. A triangle in the corner of each instance tells that it runs. The application has two modes, as its instances do: it is edited, then it runs, and it is not edited while it runs. Until it is stopped, its instances can be selected, and their properties read under the edition zone, but nothing is dropped, moved, linked, renamed, recolored, added or removed in the edition zone, nor any property edited: its context menu and the Delete key only tell, in the status bar, that the application has to be stopped first. **New** and **Open...** are disabled meanwhile, as is **Start**; a refusal to start is told in a message box, and the application is edited again. While it runs, its properties change, from the values of the user its settings files give them, or from what its components do, and what changes those the files hold is written to them; **Stop** gives every property the value it had when it started, the one of the edition, and the project is saved with these values, even while it runs: running it does not change it. The status bar tells when a file cannot be read or written.

`ExternalPlugin`, opened with **Open plugin...** from `<build>/external-plugins` (`<build>/external-plugins/<config>` with Visual Studio), or listed from the start with `NebulaStudio --plugin-dir <that directory>`, has a pair to bind: the `display` a `Ticker` provides and the `content` a `Window` consumes, which it shows in a window. Its `ProfileSelector` provides a `display` too, the list of its `profiles` in which the user chooses its `profile`: once a settings file holds that `profile`, the application shows the one chosen the next time it runs.

It is built in `<build>/bin` (`<build>/bin/<config>` with Visual Studio), next to the plugins.

Its theme is the one of Nebula, space: the night sky and the violets, blues and pinks of a nebula. It is dark or light as the system is, and changes with it while NebulaStudio is open; it is dark when the system does not tell. The names in the edition zone, the titles of the groups of properties and the name on the splash screen are written in [Space Grotesk](https://github.com/floriankarsten/space-grotesk), the rest in the font of the system. Each text and its background, and the edges of what can be used, meet the contrasts of WCAG 2.1 level AA: 4.5 to 1 for a text, 3 to 1 for an edge; so does the title of a zone, written in black or white, whichever contrasts more with its color, and the name of an instance given a color. The zones are added in the colors of the theme, in turn. Its colors are defined once for each scheme, in `NebulaStudio/src/Theme.cpp`, and every window and item takes its own from them.

The logo of Nebula, a nebula crossed by three astres bound to one another, is on its splash screen, it is the icon of its windows and of their buttons on the taskbar, and, on Windows, the one of `NebulaStudio.exe` in the file explorer and where it is pinned. It is drawn in `NebulaStudio/resources/nebula.svg`. Each term of the [glossary](docs/glossary.md) has its icon as well, drawn in `NebulaStudio/resources/icons/` in the colors of the theme, which read over its dark scheme as over its light one: the Univers on **New**, the Galaxy on **Add zone**, the Astre beside each component of their list, and the Comet on **Add settings file**, **Edit settings file...** and the dialog of a settings file.

---

## 📁 Project File

NebulaStudio saves what is in its edition zone to a `.nbp` (Nebula Project) file, and opens it again, from its **File** menu: **New**, **Open...**, **Save** and **Save As...**. Unless another directory is chosen, the projects are saved to, and looked for in, `scene/` next to NebulaStudio, created the first time one is saved: `<build>/bin/scene` (`<build>/bin/<config>/scene` with Visual Studio). A project that has a file is saved and looked for next to it instead. A project file given on its command line is opened as soon as its main window shows: `NebulaStudio my.nbp`. Before the application is replaced, or NebulaStudio closed, it asks whether to save it when it changed since it was saved or opened.

A project file is a JSON object:

```json
{
    "version": 1,
    "plugins": [
        { "id": "FabienDG.Nebula.ExternalPlugin", "file": "../../external-plugins/Debug/ExternalPlugin.nbdl" }
    ],
    "components": [
        {
            "name": "Ticker1",
            "type": "FabienDG.Nebula.ExternalPlugin.Ticker",
            "x": 200,
            "y": 120,
            "color": "#804e79a7",
            "properties": { "interval": 250 },
            "objects": [
                { "name": "Display", "properties": { "digitCount": 8, "windowTitle": "Clock" } }
            ]
        },
        { "name": "Window1", "type": "FabienDG.Nebula.ExternalPlugin.Window", "x": 500, "y": 160, "properties": {} }
    ],
    "bindings": [
        { "consumer": "Window1", "consumed": "content", "provider": "Ticker1", "provided": "display" }
    ],
    "settings": [
        {
            "file": "user.json",
            "directory": "",
            "x": 200,
            "y": 360,
            "entries": [
                { "component": "Ticker1", "property": "interval" },
                { "component": "Ticker1", "object": "Display", "property": "digitCount" }
            ]
        }
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
| `settings`   | Its settings files, written when it has some: the name of the `file`, its `directory`, where it is in the edition zone, and its `entries`, the properties it holds |
| `zones`      | The colored zones of the edition zone                                                                   |

`x`, `y`, `color` and `zones` are only for NebulaStudio: an application runs without them. The `color` of a component is written `#aarrggbb`, its opacity first: `#804e79a7` is half translucent; `#rrggbb` is read too, as an opaque color.

An entry of a settings file is a property of a `component`, or of one of its objects, given by the `object` name it was added with and, when it is not the first object of the component added with that name, its `index` among them, from 0. The `file` of each is its own: a second file of the same name, whatever its case, is reported when the project is opened, and keeps the name it was given then. The `directory` of the file is the configuration directory of the user when it is empty, `QStandardPaths::AppConfigLocation` of the application that runs (under `%LOCALAPPDATA%` on Windows), and is read from the directory of the application when it is relative. The file holds the values of the user under the names of the components, as a project holds them:

```json
{
    "Ticker1": {
        "properties": { "interval": 250 },
        "objects": [ { "name": "Display", "properties": { "digitCount": 8 } } ]
    }
}
```

What else the file holds is kept, so that applications can share one. Run from NebulaStudio, an application writes its files with an empty directory to the one of NebulaStudio, not to the one of the application delivered: what is tried in NebulaStudio does not change what its users have.

A plugin of a project is looked for by its `id` first, among the plugins of `plugins/` and of the `--plugin-dir` directories, and loaded when it is not yet; its `file` is only looked at when none of them has that `id`. A project opens wherever its plugins are, then, given their directory.

The plugin root is the directory of NebulaStudio unless another one is given on its command line: `NebulaStudio --plugin-root <directory> my.nbp`; it is the directory of the application that runs a project unless `--plugin-root` or `Universe::setPluginRoot()` sets it. The files of the plugins are written relative to it, so that a project is opened again wherever NebulaStudio and its plugins are moved to together, or wherever the plugins alone are, given their root. A file on another drive than the root cannot be relative to it and is written whole; a file written whole is read as it is.

The `properties` of a component are those that can be written and whose value JSON holds as it is: numbers, booleans and texts, and whatever is read back from its text, as dates are.

Its `objects`, written when it has some, are those it added with `addPropertiesForQObject()`, in the order it added them: the `name` each was added with, empty when it has none, and its `properties`, chosen as those of the component are. When the project is opened, each is given to the first object of that name not given one yet; an object the component no longer has is reported as missing.

Opening a project tells what could not be restored, a plugin that is no longer where it was, say, and opens the rest. The file is then no longer the one **Save** writes to, which would write over what is missing.

---

## ▶️ Running an Application

A project is an application: `Nebula::Universe` instantiates its components as the project defines them and runs them. This is what NebulaStudio does when it starts one, and all the `main` of an application has to do:

```cpp
#include <QApplication>

#include "Nebula/Application/Universe.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv); // A QCoreApplication is enough when no component shows a widget

    Nebula::Universe application;
    if(!application.open(QStringLiteral("scene/my.nbp")) || !application.start())
    {
        qCritical().noquote() << application.errorString();
        return 1;
    }
    return QApplication::exec(); // The application is stopped when it is destroyed
}
```

It links to `Nebula::Core` only: the plugins of the project are loaded when it is opened, from `plugins/` next to the application or from the file the project gives, relative to the plugin root.

Its command line can take the options NebulaStudio takes, `--plugin-dir <directory>`, as many times as needed, and `--plugin-root <directory>`: `Universe::addCommandLineOptions()` adds them to a `QCommandLineParser`, and `applyCommandLine()`, before the project is opened, scans the directories and sets the root.

```cpp
QCommandLineParser parser;
parser.addHelpOption();
Nebula::Universe::addCommandLineOptions(parser);
parser.addPositionalArgument(QStringLiteral("project"), QStringLiteral("Project file to run."));
parser.process(app);

Nebula::Universe application;
application.applyCommandLine(parser); // Before open(): the plugins of the project are found in those directories too
```

| Function                | What it does                                                                                              |
| ----------------------- | --------------------------------------------------------------------------------------------------------- |
| `open(filePath)`        | Loads the plugins of the project, instantiates its components, sets their properties, binds their interfaces and reads its settings files. `false`, with `errorString()` set, when the file cannot be read or anything could not be restored |
| `start()`               | Keeps the values of the properties of its components and of their objects, those of the edition, then gives the properties its settings files hold the values of the user, and starts its components, in the order of the project. `false`, with `errorString()` set, when one refuses: those it started are stopped again, and it is edited again, as `stop()` leaves it. Does nothing while it runs |
| `stop()`                | Stops its components, the last one first, has its settings files write the values of the user, then gives every property the value of the edition again. It can be started again |
| `isRunning()`           | Whether it was started and not stopped since; `runningChanged()` is emitted when it changes. While it runs, it is not assembled: `instantiate()`, `rename()` and `remove()` refuse, and `save()` writes the values of the edition |
| `components()`, `component(name)` | Its instances, to reach one from the `main`: `application.component(QStringLiteral("Greeter1"))` |
| `setPluginRoot(directory)` | The directory the files of the plugins are relative to in a project; the directory of the application by default |
| `addCommandLineOptions(parser)`, `applyCommandLine(parser)` | Add `--plugin-dir` and `--plugin-root` to a `QCommandLineParser`, then do what they ask once it processed the command line: a `--plugin-dir` that is not a directory is skipped, with a warning |

An application can also be assembled without a project, while it does not run: `instantiate(componentId)` creates a component in it, `rename(component, name)` names it, unless the name is empty or the one of another instance, `Component::bindInterface()` binds it, `remove(component)` destroys it, which unbinds what was bound to its interfaces, `unloadPlugin(pluginId)` removes the components of a plugin, those `componentsOf(pluginId)` lists, then unloads it, unless the application runs, and `save()` returns the project of what was assembled. `read()` and `load()` are the two halves of `open()`, for who needs the project itself, as NebulaStudio does for what it adds to it.

Its settings files, `settingsFiles()`, keep the values the user gives some properties, from one run to the next; `Nebula::SettingsFile` is one of them. When the application starts, before any of its components is started, each file gives the properties it holds the values it has for them; a file that does not exist yet is first written with the values of the project, their default values, and a file that cannot be read is set aside, a `.bak` added to its name, then written again. While the application runs, what changes these properties is written to their files, changes made together being written once; when it stops, each file is written a last time, then the properties have their default values again, which `save()` writes, even while it runs. `settingsFileFailed()` tells when a file cannot be read or written: the application runs all the same. `addSettingsFile()` adds a file, named `settings.json`, or `settings2.json`, `settings3.json`... when another file has that name, `settingsFile(entry)` tells which one holds a property, `settingsFile(fileName)` which one has a name, whatever its case, and `removeSettingsFile()` removes one; a `SettingsFile` has its `setFileName()`, which refuses the name of another file of the application, whatever its case, `setDirectory()`, `add({component, object, property})`, the object being the component itself or one of its objects, `remove()`, and `writeDefaults()`, which writes the values of the project to its file while the application does not run.

The [exemple](exemple/) folder holds `ExampleUniverseApp`, which runs the project given on its command line until its last window is closed: assemble a `Ticker` and a `Window` in NebulaStudio, save them as `scene/clock.nbp`, then run `ExampleUniverseApp scene/clock.nbp` from `<build>/bin` (`<build>/bin/<config>` with Visual Studio). It takes `--plugin-dir` and `--plugin-root` too: `ExampleUniverseApp --plugin-dir ../external-plugins scene/clock.nbp` finds `ExternalPlugin` there even when the project no longer leads to its file (`../../external-plugins/<config>` with Visual Studio). With a `ProfileSelector` in place of the `Ticker`, and a settings file that holds its `profile`, the profile chosen in the window is the one it shows the next time.

---

## 🤝 Contributing

The coding conventions the code follows, from formatting and naming to commits and pull requests, are described in [CONVENTIONS.md](CONVENTIONS.md) (in French).

---

## 📄 License

Nebula is licensed under the [Mozilla Public License 2.0](LICENSE). Copyright (C) 2026 Fabien Di Grande.

- Anyone may use, modify and distribute Nebula, in a product of any kind, commercial or not.
- A plugin or an application built on Nebula keeps the license its author chooses, open or closed.
- A file of Nebula stays under the MPL, modified or not: whoever distributes Nebula, as source or as binaries, says where the source of these files is found, with the changes made to them.
- The header `nebula_add_plugin()` generates for a plugin comes from a file of Nebula and stays under the MPL too; the sources of the plugin do not.

This summary does not replace the [license](LICENSE). Each source file carries a two-line notice (copyright and `SPDX-License-Identifier: MPL-2.0`); for the files that cannot, such as `plugin.json.in`, the notice is this one:

> This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

The license covers the files of this repository: Qt and the `CMakeUtils` submodule are separate works, under their own terms. So is Space Grotesk, the font of the titles of NebulaStudio, which `NebulaStudio.exe` holds: it is under the [SIL Open Font License 1.1](NebulaStudio/resources/fonts/OFL.txt), copied next to NebulaStudio in the package.
