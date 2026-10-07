#include <QApplication>
#include <QDir>
#include <QTextStream>
#include <memory>

#include "Component/ComponentFactory.h"
#include "Interfaces/IWidget.h"
#include "Plugin/PluginManager.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    QTextStream err(stderr);

    AppForge::PluginManager plugins;
    for(const AppForge::PluginInfo& info : plugins.scan())
    {
        plugins.load(info.id);
    }

    const AppForge::ComponentFactory& factory = AppForge::ComponentFactory::instance();
    const std::unique_ptr<AppForge::Component> label =
        factory.create(QStringLiteral("Shuko83.AppForge.WidgetPlugin.Label"));
    const std::unique_ptr<AppForge::Component> window =
        factory.create(QStringLiteral("Shuko83.AppForge.WidgetPlugin.Window"));
    if(label == nullptr || window == nullptr)
    {
        err << "WidgetPlugin is not in " << QDir::toNativeSeparators(AppForge::PluginManager::defaultDirectory())
            << '\n';
        return 1;
    }
    label->setProperty("text", QStringLiteral("Hello from a Label, shown by a Window"));
    window->setProperty("title", QStringLiteral("WidgetApp"));

    // The pointer to the IWidget the Label provides is given to the Window, which shows its widget while it runs.
    window->bindInterface<IWidget>(*label);

    for(AppForge::Component* component : {label.get(), window.get()})
    {
        component->initialize();
        component->start();
    }
    // Ends when the window is closed.
    return QApplication::exec();
}
