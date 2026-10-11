#include <QCoreApplication>
#include <QMetaProperty>
#include <QTextStream>
#include <memory>
#include <optional>

#include "Component/ComponentFactory.h"
#include "Plugin/PluginManager.h"

namespace
{

QString toString(AppForge::ComponentState state)
{
    switch(state)
    {
    case AppForge::ComponentState::Initializing:
        return QStringLiteral("Initializing");
    case AppForge::ComponentState::Ready:
        return QStringLiteral("Ready");
    case AppForge::ComponentState::Running:
        return QStringLiteral("Running");
    }
    return {};
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    // scan() reads the metadata without loading the plugins; load() reads the build date and registers the components.
    AppForge::PluginManager plugins;
    for(const AppForge::PluginInfo& info : plugins.scan())
    {
        out << "Plugin " << info.id << ' ' << info.version.toString() << ": " << info.description << '\n';
        if(!plugins.load(info.id))
        {
            out << "  not loaded: " << plugins.errorString() << '\n';
            continue;
        }
        if(const std::optional<AppForge::PluginInfo> loaded = plugins.plugin(info.id))
        {
            out << "  built " << loaded->buildDate.toString(Qt::ISODate) << '\n';
        }
    }

    const AppForge::ComponentFactory& factory = AppForge::ComponentFactory::instance();
    for(const AppForge::ComponentInfo& component : factory.components())
    {
        out << "\nComponent " << component.id << " [" << component.category << "]: " << component.description << '\n';
        for(const QMetaProperty& property : component.properties)
        {
            out << "  " << property.name() << ": " << property.typeName()
                << (property.isWritable() ? "" : ", read-only") << '\n';
        }
    }

    // The factory is the only way to create a component: `new Greeter` does not compile.
    const std::unique_ptr<AppForge::Component> greeter =
        factory.create(QStringLiteral("FabienDG.AppForge.ExamplePlugin.Greeter"));
    if(greeter == nullptr)
    {
        return 1;
    }
    QObject::connect(greeter.get(), &AppForge::Component::stateChanged,
                     [&out](AppForge::ComponentState state) { out << "  -> " << toString(state) << '\n'; });
    QObject::connect(greeter.get(), &AppForge::Component::interfaceAvailable,
                     [&out](const QString& name) { out << "  -> " << name << " available\n"; });
    QObject::connect(greeter.get(), &AppForge::Component::interfaceUnavailable,
                     [&out](const QString& name) { out << "  -> " << name << " unavailable\n"; });

    out << "\nGreeter created: " << toString(greeter->state()) << '\n';
    // Its interfaces are declared as soon as it is built; the Greeter only provides its own while it runs.
    for(const AppForge::InterfaceInfo& provided : greeter->providedInterfaces())
    {
        out << "  provides " << provided.name << ": " << provided.type.name()
            << (provided.available ? "" : ", unavailable") << '\n';
    }
    greeter->setProperty("recipient", QStringLiteral("AppForge"));
    greeter->initialize();
    greeter->start();
    out << "  greeting: " << greeter->property("greeting").toString() << '\n';
    greeter->stop();
    return 0;
}
