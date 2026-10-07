#include "WindowLogic.h"

#include <QVBoxLayout>

#include "Window.h"

WindowLogic::WindowLogic(Window& window) : QObject(&window), m_window(window)
{
    connect(&m_window, &AppForge::Component::stateChanged, this, &WindowLogic::onStateChanged);
    // Bound or unbound while it runs: shows its new content, or none.
    connect(&m_window, &AppForge::Component::consumedInterfaceChanged, this,
            [this]
            {
                if(m_frame != nullptr)
                {
                    showContent();
                }
            });
    connect(&m_window, &Window::titleChanged, this,
            [this](const QString& title)
            {
                if(m_frame != nullptr)
                {
                    m_frame->setWindowTitle(title);
                }
            });
}

WindowLogic::~WindowLogic()
{
    giveBackContent();
}

void WindowLogic::onStateChanged(AppForge::ComponentState state)
{
    if(state == AppForge::ComponentState::Running)
    {
        m_frame = std::make_unique<QWidget>();
        m_frame->setWindowTitle(m_window.title());
        m_frame->resize(320, 120);
        new QVBoxLayout(m_frame.get());
        showContent();
        m_frame->show();
    }
    else
    {
        giveBackContent();
        m_frame.reset();
    }
}

void WindowLogic::showContent()
{
    giveBackContent();
    IWidget* content = m_window.content();
    m_content = content != nullptr ? content->widget() : nullptr;
    if(m_content != nullptr)
    {
        m_frame->layout()->addWidget(m_content);
    }
}

void WindowLogic::giveBackContent()
{
    if(m_content != nullptr)
    {
        m_content->setParent(nullptr);
    }
    m_content = nullptr;
}
