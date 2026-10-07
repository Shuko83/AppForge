#pragma once

#include <QLabel>
#include <QObject>
#include <QPointer>

#include "Interfaces/IWidget.h"

class Label;

// Logic of a Label, built with it by WidgetPlugin: the IWidget it provides, a QLabel showing its text.
class LabelLogic : public QObject, public IWidget
{
    Q_OBJECT

  public:
    // A child of label, destroyed with it.
    explicit LabelLogic(Label& label);
    ~LabelLogic() override;

    [[nodiscard]] QWidget* widget() override;

  private:
    // Unparented, unless a component consuming it shows it, which gives it back; destroyed with it otherwise.
    QPointer<QLabel> m_widget;
};
