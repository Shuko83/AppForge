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

A plugin is declared with `appforge_add_plugin`, available once `Core` is added. It needs no plugin class: CMake generates the rest.

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
| `buildDate`   | UTC date of the last build in which a source or header of the plugin changed |

`APPFORGE_PLUGIN_ID` holds the id while the plugin compiles.

`AppForge::PluginManager` finds and loads the plugins:

```cpp
AppForge::PluginManager plugins;
for(const AppForge::PluginInfo& info : plugins.scan()) // <application dir>/plugins, nothing is loaded
{
    qInfo() << info.id << info.buildDate << info.description;
}
plugins.load(QStringLiteral("Shuko83.AppForge.Network"));
```

`scan()` skips, with a warning, the files that are not AppForge plugins, the plugins built against another major version of `Core` or a newer one, and the ids already found. A loaded plugin stays loaded until the application exits.

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
