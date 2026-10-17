// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <functional>
#include <type_traits>
#include <utility>

#include "Core_export.h"
#include "Nebula/Component/IComponent.h"
#include "Nebula/Component/PropertyObject.h"

namespace Nebula
{

class ComponentFactory;
class Plugin;

// Base of every component: declare its properties with Q_PROPERTY, the interfaces it provides and consumes with
// provideInterface() and consumeInterface(), and override the on*() hooks.
// Only its plugin builds a component, when ComponentFactory creates it (see NEBULA_REGISTER_COMPONENT): elsewhere, a
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
    bool start() override;
    void stop() override;

    [[nodiscard]] QList<InterfaceInfo> providedInterfaces() const override;
    [[nodiscard]] QList<InterfaceInfo> consumedInterfaces() const override;

    // Binds name, which it consumes, to providedName, which provider provides: it has its pointer whenever provider
    // makes it available, until unbindInterface() or the destruction of one of them. Replaces what name was bound to.
    // Returns false, changing nothing, when it does not consume name, provider does not provide providedName, or
    // they are not of the same type.
    bool bindInterface(const QString& name, Component& provider, const QString& providedName);
    // name, which it consumes, becomes unavailable and is bound to nothing.
    void unbindInterface(const QString& name);
    // What name is bound to: nothing when it is unbound, or not an interface it consumes.
    [[nodiscard]] InterfaceBinding binding(const QString& name) const;

    // The objects added with addPropertiesForQObject() that still live, in the order they were added.
    [[nodiscard]] QList<PropertyObject> propertyObjects() const;

  signals:
    void stateChanged(Nebula::ComponentState state);
    // The pointer of name, which it provides, can be retrieved with providedInterface().
    void interfaceAvailable(const QString& name);
    // The pointer retrieved from name, which it provides, must no longer be used: the component withdrew it, is about
    // to replace it, or is being destroyed.
    void interfaceUnavailable(const QString& name);
    // It declared an interface, or one it provides or consumes became available or unavailable.
    void interfacesChanged();
    // An object was added to propertyObjects(), or one of them was destroyed.
    void propertyObjectsChanged();

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

    // Declares that it consumes the interface name, of type T, through member, one of its members: nullptr, until
    // name is bound to an interface another component provides (see bindInterface()) and that one is available.
    // onAvailable is called once member is set; onUnavailable before it is set back to nullptr, the last time what it
    // points to can be used: when the provider withdraws or replaces its pointer, when name is unbound, and when one
    // of the two components is destroyed.
    // Returns false, with a warning, when name is already consumed.
    template <typename T>
    bool consumeInterface(const QString& name, T*& member, std::function<void()> onAvailable = {},
                          std::function<void()> onUnavailable = {})
    {
        return consume(
            name, QMetaType::fromType<T*>(), [&member](void* pointer) { member = static_cast<T*>(pointer); },
            std::move(onAvailable), std::move(onUnavailable));
    }

    // Has the properties of object, see PropertyObject::properties(), edited and saved with its own: NebulaStudio edits
    // them under those of the component, and a project holds their values. Under name when it is not empty, as a group
    // of their own; the objects added with the same name share a group. For as long as object lives: it need not
    // outlive the component. A component adds its objects when it is built, so that a project finds them when it is
    // opened. Does nothing, with a warning, when object is nullptr, the component itself, or already added.
    void addPropertiesForQObject(QObject* object, const QString& name = {});

    // Returning false from onStart() leaves the state unchanged.
    virtual bool onStart();
    virtual void onStop();

  private:
    friend class ComponentFactory;
    friend class Plugin; // Calls releaseInterfaces()

    // Only implemented by Plugin, which keeps a component from being instantiated anywhere else.
    virtual void builtByPlugin() = 0;

    struct ProvidedInterface
    {
        QString name;
        QMetaType type;
        void* pointer = nullptr; // nullptr while unavailable
    };

    struct ConsumedInterface
    {
        QString name;
        QMetaType type;
        std::function<void(void*)> setMember; // Writes the member given to consumeInterface()
        std::function<void()> onAvailable;
        std::function<void()> onUnavailable;
        bool available = false;                     // Whether the member is set
        Component* provider = nullptr;              // nullptr while unbound
        QString providedName;                       // What it is bound to in provider
        QList<QMetaObject::Connection> connections; // To provider, while bound
    };

    [[nodiscard]] void* providedPointer(const QString& name, QMetaType type) const override;
    // provideInterface(), type being the one of pointer.
    bool provide(const QString& name, QMetaType type, void* pointer);
    // consumeInterface(), type being the one of the member setMember writes.
    bool consume(const QString& name, QMetaType type, std::function<void(void*)> setMember,
                 std::function<void()> onAvailable, std::function<void()> onUnavailable);
    // Gives name, which it consumes, the pointer of what it is bound to, when there is one.
    void deliverInterface(const QString& name);
    // Takes its pointer back from name, which it consumes, when it has one.
    void withdrawInterface(const QString& name);
    // Ends what it consumes and provides while it is still whole: Plugin calls it before destroying the component, as
    // the callbacks of what it consumes, and what it provides, may be gone once its destructors run.
    void releaseInterfaces();

    void setState(ComponentState state);

    QString m_componentId; // Set by ComponentFactory
    ComponentState m_state = ComponentState::Editing;
    QList<ProvidedInterface> m_providedInterfaces; // In the order they were declared
    QList<ConsumedInterface> m_consumedInterfaces; // In the order they were declared
    QList<PropertyObject> m_propertyObjects;       // In the order they were added
};

} // namespace Nebula
