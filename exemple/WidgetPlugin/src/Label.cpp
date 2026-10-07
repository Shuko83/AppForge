#include "Label.h"

#include "WidgetPlugin.h"

APPFORGE_REGISTER_COMPONENT(WidgetPlugin, Label);

Label::Label()
{
    provideInterface<IWidget>(m_widget);
}

QString Label::text() const
{
    return m_text;
}

void Label::setText(const QString& text)
{
    if(m_text == text)
    {
        return;
    }
    m_text = text;
    emit textChanged(m_text);
}

void Label::setWidget(IWidget* widget)
{
    m_widget = widget;
}
