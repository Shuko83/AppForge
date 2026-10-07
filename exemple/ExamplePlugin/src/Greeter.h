#pragma once

#include <QString>

#include "Component/Component.h"

// Exposes a greeting for its recipient, written when it starts by GreeterLogic, which ExamplePlugin builds it with.
class Greeter : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Greets its recipient when it starts")
    Q_CLASSINFO("category", "Example")
    Q_PROPERTY(QString recipient READ recipient WRITE setRecipient NOTIFY recipientChanged)
    Q_PROPERTY(QString greeting READ greeting NOTIFY greetingChanged)

  public:
    [[nodiscard]] QString recipient() const;
    void setRecipient(const QString& recipient);

    [[nodiscard]] QString greeting() const;
    // Not a property setter: only its logic writes the greeting.
    void setGreeting(const QString& greeting);

  signals:
    void recipientChanged(const QString& recipient);
    void greetingChanged(const QString& greeting);

  private:
    QString m_recipient = QStringLiteral("World");
    QString m_greeting;
};
