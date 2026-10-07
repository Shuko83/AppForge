#pragma once

#include <QString>

#include "Component/Component.h"
#include "Interfaces/IWidget.h"

// Consumes an IWidget, shown in a window by WindowLogic, which WidgetPlugin builds it with, while it runs.
class Window : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Shows the widget it consumes in a window while it runs")
    Q_CLASSINFO("category", "Example")
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

  public:
    Window();

    [[nodiscard]] QString title() const;
    void setTitle(const QString& title);

    // The IWidget it consumes, nullptr until it is bound to a component providing one.
    [[nodiscard]] IWidget* content() const;

  signals:
    void titleChanged(const QString& title);

  private:
    QString m_title = QStringLiteral("Window");
    IWidget* m_content = nullptr; // Consumed
};
