#include "Window.h"

#include <QWidget>

#include "ExternalPlugin.h"

APPFORGE_REGISTER_COMPONENT(ExternalPlugin, Window);

Window::Window()
{
    // m_content is set while what "content" is bound to is available; its logic follows through the signals.
    consumeInterface<QWidget>(
        QStringLiteral("content"), m_content, [this] { emit contentAvailable(); },
        [this] { emit contentUnavailable(); });
}

QWidget* Window::content() const
{
    return m_content;
}
