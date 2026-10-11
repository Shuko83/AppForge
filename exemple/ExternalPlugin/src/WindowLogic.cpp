#include "WindowLogic.h"

#include <QVBoxLayout>

#include "Window.h"

WindowLogic::WindowLogic(Window& window) : QObject(&window), m_window(window)
{
    new QVBoxLayout(&m_widget);
    // The application it is assembled in decides when to quit, not this window.
    m_widget.setAttribute(Qt::WA_QuitOnClose, false);
    connect(&m_window, &QObject::objectNameChanged, &m_widget, &QWidget::setWindowTitle);
    connect(&m_window, &Window::contentAvailable, this, &WindowLogic::showContent);
    connect(&m_window, &Window::contentUnavailable, this, &WindowLogic::removeContent);
}

void WindowLogic::showContent()
{
    m_widget.layout()->addWidget(m_window.content());
    m_window.content()->show(); // Hidden when it was removed from a window
    m_widget.show();
}

void WindowLogic::removeContent()
{
    m_widget.hide();
    m_widget.layout()->removeWidget(m_window.content());
    m_window.content()->setParent(nullptr);
}
