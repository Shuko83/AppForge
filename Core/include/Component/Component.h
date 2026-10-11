#pragma once

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <type_traits>

#include "Component/IComponent.h"
#include "Core_export.h"

namespace AppForge
{

class ComponentFactory;

// Base of every component: declare its properties with Q_PROPERTY and override the on*() hooks.
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

    [[nodiscard]] QList<InterfaceInfo> providedInterfaces() const override;

  signals:
    void stateChanged(AppForge::ComponentState state);
    // The component declared a new interface, see provideInterface().
    void interfaceAdded(const QString& name);
    // The pointer of name can be retrieved, with providedInterface().
    void interfaceAvailable(const QString& name);
    // The pointer retrieved from name must no longer be used: the component withdrew it, is about to replace it, or
    // is being destroyed.
    void interfaceUnavailable(const QString& name);

  protected:
    Component();

    // Provides pointer as the interface name, of type T: providedInterface<T>(name) returns it. Without a pointer, the
    // interface is declared, or withdrawn: it is listed, but unavailable. A component declares its interfaces when it
    // is built, so that they are known before it runs.
    // Replacing the pointer of an available interface emits interfaceUnavailable() first, then interfaceAvailable()
    // once it is replaced: what the previous pointer refers to must still exist during this call.
    // Returns false, with a warning, when name is already provided with another type.
    template <typename T> bool provideInterface(const QString& name, std::type_identity_t<T>* pointer = nullptr)
    {
        return provide(name, QMetaType::fromType<T*>(), const_cast<void*>(static_cast<const void*>(pointer)));
    }

    // Returning false from onInitialize() or onStart() leaves the state unchanged.
    virtual bool onInitialize();
    virtual bool onStart();
    virtual void onStop();

  private:
    friend class ComponentFactory;

    // Only implemented by Plugin, which keeps a component from being instantiated anywhere else.
    virtual void builtByPlugin() = 0;

    struct ProvidedInterface
    {
        QString name;
        QMetaType type;
        void* pointer = nullptr; // nullptr while unavailable
    };

    [[nodiscard]] void* providedPointer(const QString& name, QMetaType type) const override;
    // provideInterface(), type being the one of pointer.
    bool provide(const QString& name, QMetaType type, void* pointer);

    void setState(ComponentState state);

    QString m_componentId; // Set by ComponentFactory
    ComponentState m_state = ComponentState::Initializing;
    QList<ProvidedInterface> m_interfaces; // In the order they were declared
};

} // namespace AppForge
