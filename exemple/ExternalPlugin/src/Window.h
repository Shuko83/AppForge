// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "AppForge/Component/Component.h"

class QWidget;

// Shows the widget it consumes as the interface "content" in a window, for as long as it has it; the window is the one
// of WindowLogic, which ExternalPlugin builds it with. It needs a QApplication.
class Window : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Shows the widget it consumes in a window")
    Q_CLASSINFO("category", "Example")

  public:
    Window();

    // nullptr while it is unavailable.
    [[nodiscard]] QWidget* content() const;

  signals:
    // content() can be used.
    void contentAvailable();
    // content() must no longer be used once the slots return.
    void contentUnavailable();

  private:
    QWidget* m_content = nullptr;
};
