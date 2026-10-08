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

A plugin is declared with `appforge_add_plugin`, available once `Core` is added. CMake generates its root object and its metadata; the plugin only declares its class, which builds its components (see below).

```cmake
appforge_add_plugin(
    NAME Network
    DESCRIPTION "TCP and UDP components"   # optional
)
```

It accepts every keyword of `cmu_add_target` ([CMakeUtils](CMakeUtils/README.md)) and creates a `MODULE` library linked to `AppForge::Core`, named `<NAME>.afplugin` and written to `plugins/` next to the executables (`CMAKE_RUNTIME_OUTPUT_DIRECTORY`, else `<build>/bin`). Its metadata is embedded in the binary and read without loading it:

| Key           | Value                                                                        |
| ------------- | ---------------------------------------------------------------------------- |
| `id`          | `<cmu_organization>.<PROJECT_NAME>.<NAME>`, e.g. `Shuko83.AppForge.Network`  |
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
plugins.load(QStringLiteral("Shuko83.AppForge.Network"));
qInfo() << plugins.plugin(QStringLiteral("Shuko83.AppForge.Network"))->buildDate;
```

`scan()` skips, with a warning, the files that are not AppForge plugins, the plugins built against another major version of `Core` or a newer one, and the ids already found. `addFile(filePath)` adds a single plugin, anywhere on the disk, with the same checks; it tells why a file cannot be added through `errorString()`. Loading a plugin registers its components; a loaded plugin stays loaded until the application exits.

The [exemple](exemple/) folder holds `ExamplePlugin`, a plugin providing the `Greeter` component, and `ExampleApp`, which loads the plugins next to it, lists their components and runs a `Greeter`: run it from `<build>/bin` (`<build>/bin/<config>` with Visual Studio).

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
| `id`          | `<plugin id>.<name>`, e.g. `Shuko83.AppForge.ExamplePlugin.Greeter`      |
| `name`        | Class name, without its namespace                                        |
| `description` | `Q_CLASSINFO("description", ...)`, empty when not given                  |
| `category`    | `Q_CLASSINFO("category", ...)`, empty when not given                     |
| `properties`  | Its `Q_PROPERTY`, including those of its base classes up to `Component`  |

The factory is the only way to instantiate a component, which it has its plugin build: `new Greeter` does not compile, as a class deriving from `Component` stays abstract until its plugin builds it. A component therefore needs a default constructor and cannot be `final`; registering a class that is not a component, or lacks `Q_OBJECT`, does not compile either.

```cpp
std::unique_ptr<AppForge::Component> greeter =
    AppForge::ComponentFactory::instance().create(QStringLiteral("Shuko83.AppForge.ExamplePlugin.Greeter"));
greeter->setProperty("recipient", QStringLiteral("AppForge")); // Initializing: being configured
greeter->initialize();                                          // Ready
greeter->start();                                               // Running
greeter->stop();                                                // Ready
```

Each step calls a hook the component can override (`onInitialize()`, `onStart()`, `onStop()`); `onInitialize()` and `onStart()` can refuse it by returning `false`. `stateChanged()` is emitted on every change.

### Interfaces

A component provides interfaces to other components, and consumes interfaces from them: a component provides a widget, another one shows it. An interface comes from elsewhere, neither from the provider nor from the consumer: a header both plugins use, which declares it once, outside any namespace, with a versioned id:

```cpp
// Interfaces/IWidget.h, in a library of interfaces
class IWidget
{
  public:
    virtual ~IWidget() = default;
    virtual QWidget* widget() = 0;
};

APPFORGE_DECLARE_INTERFACE(IWidget, "Shuko83.AppForge.IWidget/1.0");
```

In its constructor, a component declares a member pointer for each interface it provides or consumes:

```cpp
Label::Label()
{
    provideInterface<IWidget>(m_widget);  // IWidget* m_widget, set by its plugin to its logic
}

Window::Window()
{
    consumeInterface<IWidget>(m_content); // IWidget* m_content, nullptr until it is bound
}
```

Binding transfers the pointer: the member of the consumer takes the value the member of the provider has then.

```cpp
window->bindInterface<IWidget>(*label);                             // Or by id, without the header:
window->bindInterface(QStringLiteral("Shuko83.AppForge.IWidget/1.0"), *label);
window->unbindInterface(QStringLiteral("Shuko83.AppForge.IWidget/1.0")); // Back to nullptr
```

- A component provides or consumes each interface once; `providedInterfaces()` and `consumedInterfaces()` list their ids.
- `bindInterface()` returns `false`, changing nothing, when the consumer does not consume the interface or the provider does not provide it; binding again replaces the provider.
- The consumer is unbound when its provider is destroyed. `consumedInterfaceChanged()` is emitted on every bind and unbind, also while it runs, for its logic to follow.

The [exemple](exemple/) folder holds `ExampleInterfaces`, the library declaring `IWidget`; `WidgetPlugin`, whose `Label` provides an `IWidget` showing its text and whose `Window` shows the `IWidget` it consumes in a window while it runs; and `WidgetApp`, which binds them.

---

## 🔨 Forge

`Forge` is the application that assembles applications from the components of the plugins, live. For now, it lists:

- the plugins of `plugins/` next to it, with a viewer of the selected one: its metadata, its state and a **Load** button; **Open plugin...** loads a plugin from anywhere on the disk;
- the components of the loaded plugins, with a viewer of the selected one: what it is and its properties;
- between them, the edition zone of the application: a component dragged from its list and dropped there is instantiated, named after its component and a number (`Greeter1`, `Greeter2`...), and shown where it was dropped. Its instances can be moved and selected.

It is built in `<build>/bin` (`<build>/bin/<config>` with Visual Studio), next to the plugins.

---

## 🧪 Tests

The [tests](tests/) folder holds the unit tests of `Core`, written with Qt Test and run by CTest:

| Test                   | What it checks                                                                                  |
| ---------------------- | ----------------------------------------------------------------------------------------------- |
| `ComponentTest`        | The life cycle of a component and its hooks; declaring, binding and unbinding interfaces         |
| `ComponentFactoryTest` | What the factory reads from the `QMetaObject` of the components, and how it has their plugin build them |
| `PluginManagerTest`    | Scanning, adding and loading the plugins, with `TestPlugin` and `NewerCorePlugin`, built for it |

```bash
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

They are built with the project, unless `-DBUILD_TESTING=OFF`. The test plugins are written to `<build>/tests/plugins`, so that neither Forge nor the examples list them, and are not installed. CTest finds the DLLs of Qt with CMake 3.22 or newer.

---

## 📁 Project File

AppForge uses `.afp` (AppForge Project) files to store application structure:

```json
{
  "version": 1,
  "plugins": ["core"],
  "components": [
    {
      "id": "comp1",
      "type": "ExampleComponent"
    }
  ],
  "bindings": []
}
