// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QSplashScreen>
#include <QTimer>

class QMouseEvent;
class QShowEvent;

// Shown while NebulaStudio starts: the logo, the name and the version of Nebula, the Qt and compiler NebulaStudio was
// built with and when, its copyright and its license, in the colors of the palette. It tells when it has been read, a
// while after it was shown or as soon as it is clicked, and leaves it to the main window to replace it then, with
// finish().
class SplashScreen : public QSplashScreen
{
    Q_OBJECT

  public:
    SplashScreen();

  signals:
    // Emitted once: two seconds after it was first shown, or when it is clicked before that.
    void read();

  protected:
    // Starts the time it stays, the first time it is shown.
    void showEvent(QShowEvent* event) override;
    // Emits read() at once, in place of hiding it.
    void mousePressEvent(QMouseEvent* event) override;

  private:
    // Emits read(), unless it already did.
    void setRead();

    QTimer m_readingTimer; // Running from its first showing until it is read
    bool m_read = false;
};
