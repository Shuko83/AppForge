#include "Greeter.h"

#include "ExamplePlugin.h"

APPFORGE_REGISTER_COMPONENT(ExamplePlugin, Greeter);

QString Greeter::recipient() const
{
    return m_recipient;
}

void Greeter::setRecipient(const QString& recipient)
{
    if(m_recipient == recipient)
    {
        return;
    }
    m_recipient = recipient;
    emit recipientChanged(m_recipient);
}

QString Greeter::greeting() const
{
    return m_greeting;
}

void Greeter::setGreeting(const QString& greeting)
{
    if(m_greeting == greeting)
    {
        return;
    }
    m_greeting = greeting;
    emit greetingChanged(m_greeting);
}
