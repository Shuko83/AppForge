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
