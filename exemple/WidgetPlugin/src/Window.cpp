#include "Window.h"

#include "WidgetPlugin.h"

APPFORGE_REGISTER_COMPONENT(WidgetPlugin, Window);

Window::Window()
{
    consumeInterface<IWidget>(m_content);
}

QString Window::title() const
{
    return m_title;
}

void Window::setTitle(const QString& title)
{
    if(m_title == title)
    {
        return;
    }
    m_title = title;
    emit titleChanged(m_title);
}

IWidget* Window::content() const
{
    return m_content;
}
