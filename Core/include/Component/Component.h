#pragma once

#include <QObject>
#include <QString>

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

  signals:
    void stateChanged(AppForge::ComponentState state);

  protected:
    Component();

    // Returning false from onInitialize() or onStart() leaves the state unchanged.
    virtual bool onInitialize();
    virtual bool onStart();
    virtual void onStop();

  private:
    friend class ComponentFactory;

    // Only implemented by Plugin, which keeps a component from being instantiated anywhere else.
    virtual void builtByPlugin() = 0;

    void setState(ComponentState state);

    QString m_componentId; // Set by ComponentFactory
    ComponentState m_state = ComponentState::Initializing;
};

} // namespace AppForge
