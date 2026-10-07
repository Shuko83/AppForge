#include "LabelLogic.h"

#include "Label.h"

LabelLogic::LabelLogic(Label& label) : QObject(&label), m_widget(new QLabel(label.text()))
{
    m_widget->setAlignment(Qt::AlignCenter);
    connect(&label, &Label::textChanged, m_widget, &QLabel::setText);
}

LabelLogic::~LabelLogic()
{
    delete m_widget;
}

QWidget* LabelLogic::widget()
{
    return m_widget;
}
