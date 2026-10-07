#pragma once

#include <QString>

#include "Component/Component.h"
#include "Interfaces/IWidget.h"

// Provides an IWidget showing its text, implemented by LabelLogic, which WidgetPlugin builds it with.
class Label : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Provides a widget showing its text")
    Q_CLASSINFO("category", "Example")
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

  public:
    Label();

    [[nodiscard]] QString text() const;
    void setText(const QString& text);

    // Not a property: set by its plugin, to its logic.
    void setWidget(IWidget* widget);

  signals:
    void textChanged(const QString& text);

  private:
    QString m_text = QStringLiteral("Label");
    IWidget* m_widget = nullptr; // Provided
};
