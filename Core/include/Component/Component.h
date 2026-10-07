#pragma once

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <vector>

#include "Component/IComponent.h"
#include "Component/Interface.h"
#include "Core_export.h"

namespace AppForge
{

class ComponentFactory;

// Base of every component: declare its properties with Q_PROPERTY, the interfaces it provides and consumes with
// provideInterface() and consumeInterface(), and override the on*() hooks.
// Only its plugin builds a component, when ComponentFactory creates it (see APPFORGE_REGISTER_COMPONENT): elsewhere, a
// class deriving from Component is abstract.
class CORE_EXPORT Component : public QObject, public IComponent
{
    Q_OBJECT
    Q_PROPERTY(QString componentId READ componentId CONSTANT)

  public:
    ~Component() override;

    [[nodiscard]] QString componentId() const override;
    [[nodiscard]] ComponentState state() const override;

    // Each call returns false, without calling its hook, when the current state does not allow it.
    bool initialize() override;
    bool start() override;
    void stop() override;

    // Ids of the interfaces declared with provideInterface() and consumeInterface(), in the order they were declared.
    [[nodiscard]] QStringList providedInterfaces() const;
    [[nodiscard]] QStringList consumedInterfaces() const;

    // Gives the component the interfaceId provider provides: the member the component declared with consumeInterface()
    // takes the pointer held by the member provider declared with provideInterface(). Replaces the interfaceId it
    // consumed, and is undone when provider is destroyed. Returns false, changing nothing, when the component does not
    // consume interfaceId or provider does not provide it.
    bool bindInterface(const QString& interfaceId, const Component& provider);
    template <Interface I> bool bindInterface(const Component& provider)
    {
        return bindInterface(AppForge::interfaceId<I>(), provider);
    }
    // Sets back to nullptr the member the component declared with consumeInterface() for interfaceId.
    void unbindInterface(const QString& interfaceId);

  signals:
    void stateChanged(AppForge::ComponentState state);
    // The member declared with consumeInterface() for interfaceId was bound or unbound.
    void consumedInterfaceChanged(const QString& interfaceId);

  protected:
    Component();

    // Returning false from onInitialize() or onStart() leaves the state unchanged.
    virtual bool onInitialize();
    virtual bool onStart();
    virtual void onStop();

    // Declares, in the constructor of the component, that it provides an I: pointer, one of its members, is what
    // bindInterface() gives the components consuming an I, read when they are bound. One I per component.
    template <Interface I> void provideInterface(I*& pointer)
    {
        addProvidedInterface(AppForge::interfaceId<I>(), [&pointer]() -> void* { return pointer; });
    }
    // Declares, in the constructor of the component, that it consumes an I: bindInterface() sets pointer, one of its
    // members, to the I another component provides; nullptr until then. One I per component.
    template <Interface I> void consumeInterface(I*& pointer)
    {
        addConsumedInterface(AppForge::interfaceId<I>(),
                             [&pointer](void* provided) { pointer = static_cast<I*>(provided); });
    }

  private:
    friend class ComponentFactory;

    // Reads the member declared with provideInterface().
    using Getter = std::function<void*()>;
    // Writes the member declared with consumeInterface(), the I* read by a Getter of the same interface.
    using Setter = std::function<void(void*)>;

    struct ProvidedInterface
    {
        QString id;
        Getter getter;
    };

    struct ConsumedInterface
    {
        QString id;
        Setter setter;
        const Component* provider = nullptr;       // nullptr while unbound
        QMetaObject::Connection providerDestroyed; // Unbinds it
    };

    // Only implemented by Plugin, which keeps a component from being instantiated anywhere else.
    virtual void builtByPlugin() = 0;

    void setState(ComponentState state);
    // Ignored, with a warning, when id is already declared.
    void addProvidedInterface(const QString& id, Getter getter);
    void addConsumedInterface(const QString& id, Setter setter);

    QString m_componentId; // Set by ComponentFactory
    ComponentState m_state = ComponentState::Initializing;
    std::vector<ProvidedInterface> m_providedInterfaces;
    std::vector<ConsumedInterface> m_consumedInterfaces;
};

} // namespace AppForge
