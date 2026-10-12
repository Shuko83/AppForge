#pragma once

#include <QString>
#include <memory>
#include <vector>

namespace AppForge
{
class Component;
class ComponentFactory;
} // namespace AppForge

// The application assembled in Forge: owns the components instantiated in it, each named, as its objectName, after its
// component and a number, e.g. Greeter1.
class Assembly
{
  public:
    explicit Assembly(const AppForge::ComponentFactory& factory);
    ~Assembly();

    Assembly(const Assembly&) = delete;
    Assembly& operator=(const Assembly&) = delete;

    // Creates componentId, in the Initializing state; nullptr when it is not registered. It is named name, as one read
    // from a project file is, unless name is empty or the name of another instance.
    AppForge::Component* instantiate(const QString& componentId, const QString& name = {});

    // In the order they were instantiated.
    [[nodiscard]] const std::vector<std::unique_ptr<AppForge::Component>>& components() const;
    // Destroys its components: nothing may refer to them any longer.
    void clear();

  private:
    // Whether an instance is named name.
    [[nodiscard]] bool contains(const QString& name) const;
    // componentName and the first number no instance is named with.
    [[nodiscard]] QString newName(const QString& componentName) const;

    const AppForge::ComponentFactory& m_factory;
    std::vector<std::unique_ptr<AppForge::Component>> m_components;
};
