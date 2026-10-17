// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Window.h"

#include <QWidget>

#include "ExternalPlugin.h"

NEBULA_REGISTER_COMPONENT(ExternalPlugin, Window);

Window::Window()
{
    // m_content is set while what "content" is bound to is available; its logic follows through the signals. Mandatory:
    // without it, the window is never shown.
    consumeInterface<QWidget>(
        QStringLiteral("content"), m_content, Nebula::InterfaceRequirement::Mandatory,
        [this] { emit contentAvailable(); }, [this] { emit contentUnavailable(); });
}

QWidget* Window::content() const
{
    return m_content;
}
