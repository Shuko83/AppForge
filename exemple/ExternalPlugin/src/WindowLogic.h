// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QObject>
#include <QWidget>

class Window;

// Logic of a Window, built with it by ExternalPlugin: owns the window, shown with the content of the Window in it for
// as long as it is available.
class WindowLogic : public QObject
{
    Q_OBJECT

  public:
    // A child of window, destroyed with it.
    explicit WindowLogic(Window& window);

  private:
    void showContent();
    // Gives the content back as it was provided: without a parent, which would delete it.
    void removeContent();

    Window& m_window;
    QWidget m_widget;
};
