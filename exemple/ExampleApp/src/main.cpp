// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCoreApplication>
#include <QMetaProperty>
#include <QTextStream>
#include <memory>
#include <optional>

#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Plugin/PluginManager.h"

namespace
{

QString toString(Nebula::ComponentState state)
{
    switch(state)
    {
    case Nebula::ComponentState::Editing:
        return QStringLiteral("Editing");
    case Nebula::ComponentState::Running:
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
    Nebula::PluginManager plugins;
    for(const Nebula::PluginInfo& info : plugins.scan())
    {
        out << "Plugin " << info.id << ' ' << info.version.toString() << ": " << info.description << '\n';
        if(!plugins.load(info.id))
        {
            out << "  not loaded: " << plugins.errorString() << '\n';
            continue;
        }
        if(const std::optional<Nebula::PluginInfo> loaded = plugins.plugin(info.id))
        {
            out << "  built " << loaded->buildDate.toString(Qt::ISODate) << '\n';
        }
    }

    const Nebula::ComponentFactory& factory = Nebula::ComponentFactory::instance();
    for(const Nebula::ComponentInfo& component : factory.components())
    {
        out << "\nComponent " << component.id << " [" << component.category << "]: " << component.description << '\n';
        for(const QMetaProperty& property : component.properties)
        {
            out << "  " << property.name() << ": " << property.typeName()
                << (property.isWritable() ? "" : ", read-only") << '\n';
        }
    }

    // The factory is the only way to create a component: `new Greeter` does not compile.
    const std::unique_ptr<Nebula::Component> greeter =
        factory.create(QStringLiteral("FabienDG.Nebula.ExamplePlugin.Greeter"));
    if(greeter == nullptr)
    {
        return 1;
    }
    QObject::connect(greeter.get(), &Nebula::Component::stateChanged,
                     [&out](Nebula::ComponentState state) { out << "  -> " << toString(state) << '\n'; });
    QObject::connect(greeter.get(), &Nebula::Component::interfaceAvailable,
                     [&out](const QString& name) { out << "  -> " << name << " available\n"; });
    QObject::connect(greeter.get(), &Nebula::Component::interfaceUnavailable,
                     [&out](const QString& name) { out << "  -> " << name << " unavailable\n"; });

    out << "\nGreeter created: " << toString(greeter->state()) << '\n';
    // Its interfaces are declared as soon as it is built; the Greeter only provides its own while it runs.
    for(const Nebula::InterfaceInfo& provided : greeter->providedInterfaces())
    {
        out << "  provides " << provided.name << ": " << provided.type.name()
            << (provided.available ? "" : ", unavailable") << '\n';
    }
    greeter->setProperty("recipient", QStringLiteral("Nebula"));
    greeter->start();
    out << "  greeting: " << greeter->property("greeting").toString() << '\n';
    greeter->stop();
    return 0;
}
