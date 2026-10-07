#pragma once

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <memory>

#include "Component/IComponent.h"

class Window;

// Logic of a Window, built with it by WidgetPlugin: while the Window runs, shows in a window the widget it consumes.
class WindowLogic : public QObject
{
    Q_OBJECT

  public:
    // A child of window, destroyed with it.
    explicit WindowLogic(Window& window);
    ~WindowLogic() override;

  private:
    void onStateChanged(AppForge::ComponentState state);
    // Shows the widget the Window consumes now, instead of the one shown.
    void showContent();
    // Gives back, unparented, the widget shown to the component providing it.
    void giveBackContent();

    Window& m_window;
    std::unique_ptr<QWidget> m_frame; // The window, while the Window runs
    QPointer<QWidget> m_content;      // Shown in m_frame, owned by the component providing it
};
