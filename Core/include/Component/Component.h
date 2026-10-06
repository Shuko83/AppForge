#pragma once

#include <QObject>
#include <QString>

#include "Component/IComponent.h"
#include "Core_export.h"

namespace AppForge
{

// Base implementation: owns the name and the lifecycle; custom components override the on*() hooks.
class CORE_EXPORT Component : public QObject, public IComponent
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)

  public:
    explicit Component(QString name, QObject* parent = nullptr);
    ~Component() override;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] ComponentState state() const override;

    // Each call returns false, without calling its hook, when the current state does not allow it.
    bool initialize() override;
    bool start() override;
    void stop() override;

  signals:
    void stateChanged(AppForge::ComponentState state);

  protected:
    // Returning false from onInitialize() or onStart() leaves the state unchanged.
    virtual bool onInitialize();
    virtual bool onStart();
    virtual void onStop();

  private:
    void setState(ComponentState state);

    QString m_name;
    ComponentState m_state = ComponentState::Created;
};

} // namespace AppForge
