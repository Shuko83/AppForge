#pragma once

#include "Component/Interface.h"

class QWidget;

// A widget, provided by a component to the components that show it.
class IWidget
{
  public:
    virtual ~IWidget() = default;

    // Owned by the provider: a component showing it gives it back, unparented, once it stops showing it.
    [[nodiscard]] virtual QWidget* widget() = 0;
};

APPFORGE_DECLARE_INTERFACE(IWidget, "Shuko83.AppForge.IWidget/1.0");
