// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <memory>

#include "Nebula/Component/ComponentFactory.h"
#include "Options.h"
#include "Probe.h"
#include "TestComponents.h"

using Nebula::ComponentState;
using Nebula::InterfaceInfo;
using Nebula::InterfaceRequirement;

namespace
{

// A new Probe, as its plugin builds it.
std::unique_ptr<Probe> createProbe()
{
    std::unique_ptr<Nebula::Component> component =
        Nebula::ComponentFactory::instance().create(testComponentId(QStringLiteral("Probe")));
    auto* probe = qobject_cast<Probe*>(component.get());
    if(probe == nullptr)
    {
        return nullptr;
    }
    component.release();
    return std::unique_ptr<Probe>(probe);
}

// Appends "available <name>" and "unavailable <name>" to recorded, which outlives component, for each
// interfaceAvailable() and interfaceUnavailable() it emits.
void recordInterfaceSignals(Nebula::Component& component, QStringList& recorded)
{
    QObject::connect(&component, &Nebula::Component::interfaceAvailable, &component,
                     [&recorded](const QString& name) { recorded.append(QStringLiteral("available ") + name); });
    QObject::connect(&component, &Nebula::Component::interfaceUnavailable, &component,
                     [&recorded](const QString& name) { recorded.append(QStringLiteral("unavailable ") + name); });
}

} // namespace

// Component: its two states, the interfaces it provides and consumes, how they are bound, and the objects whose
// properties it adds to its own.
class TestComponent : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void start_data();
    void start();
    void startWhileRunningIsRefused();
    void stopWhileEditingDoesNothing();

    void provideInterface_data();
    void provideInterface();
    void providedInterface_data();
    void providedInterface();
    void provideWithAnotherTypeIsRefused();
    void consumeTwiceIsRefused();
    void consumedInterfaces();

    void bindInterface_data();
    void bindInterface();
    void boundInterfaceFollowsItsProvider();
    void bindingToAnAvailableInterfaceDeliversIt();
    void bindingAgainReplacesTheProvider();
    void unbindInterface();
    void destroyedProviderUnbinds();
    void destroyedConsumerLeavesItsProvider();
    void unboundMandatoryInterfaces_data();
    void unboundMandatoryInterfaces();

    void addPropertiesForQObject();
    void addPropertiesForQObjectRefuses_data();
    void addPropertiesForQObjectRefuses();
    void propertyObjectProperties_data();
    void propertyObjectProperties();
};

void TestComponent::initTestCase()
{
    registerTestComponents();
}

void TestComponent::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestComponent::start_data()
{
    QTest::addColumn<bool>("refuses");
    QTest::addColumn<bool>("started");
    QTest::addColumn<ComponentState>("state");
    QTest::addColumn<QStringList>("events");

    QTest::newRow("accepted") << false << true << ComponentState::Running << QStringList{"start", "stop"};
    // Its state unchanged, it is not stopped.
    QTest::newRow("refused") << true << false << ComponentState::Editing << QStringList{"start"};
}

void TestComponent::start()
{
    QFETCH(bool, refuses);
    QFETCH(bool, started);
    QFETCH(ComponentState, state);
    QFETCH(QStringList, events);

    const std::unique_ptr<Probe> probe = createProbe();
    QVERIFY(probe);
    probe->setProperty("refusesToStart", refuses);
    const QSignalSpy stateChanged(probe.get(), &Nebula::Component::stateChanged);

    QCOMPARE(probe->start(), started);
    QCOMPARE(probe->state(), state);
    QCOMPARE(stateChanged.count(), started ? 1 : 0);
    QCOMPARE(probe->starts(), started ? 1 : 0);

    probe->stop();
    QCOMPARE(probe->state(), ComponentState::Editing);
    QCOMPARE(probe->events(), events);
    QCOMPARE(stateChanged.count(), started ? 2 : 0);
    if(started)
    {
        QCOMPARE(stateChanged.last().first().value<ComponentState>(), ComponentState::Editing);
    }
}

void TestComponent::startWhileRunningIsRefused()
{
    const std::unique_ptr<Probe> probe = createProbe();
    QVERIFY(probe->start());
    probe->clearEvents();

    QVERIFY(!probe->start());
    QCOMPARE(probe->state(), ComponentState::Running);
    QVERIFY(probe->events().isEmpty()); // onStart() is not called
}

void TestComponent::stopWhileEditingDoesNothing()
{
    const std::unique_ptr<Probe> probe = createProbe();
    const QSignalSpy stateChanged(probe.get(), &Nebula::Component::stateChanged);

    probe->stop();
    QCOMPARE(probe->state(), ComponentState::Editing);
    QVERIFY(probe->events().isEmpty()); // onStop() is not called
    QCOMPARE(stateChanged.count(), 0);
}

void TestComponent::provideInterface_data()
{
    // The pointers given one after the other: 0 for nullptr, 1 and 2 for two objects.
    QTest::addColumn<QList<int>>("pointers");
    QTest::addColumn<QStringList>("emitted");
    QTest::addColumn<bool>("available");

    QTest::newRow("declared") << QList<int>{0} << QStringList{} << false;
    QTest::newRow("provided") << QList<int>{1} << QStringList{"available out"} << true;
    QTest::newRow("provided again") << QList<int>{1, 1} << QStringList{"available out"} << true;
    QTest::newRow("withdrawn") << QList<int>{1, 0} << QStringList{"available out", "unavailable out"} << false;
    QTest::newRow("withdrawn again") << QList<int>{1, 0, 0} << QStringList{"available out", "unavailable out"} << false;
    // Unavailable first: what used the former pointer lets it go before it is replaced.
    QTest::newRow("replaced") << QList<int>{1, 2} << QStringList{"available out", "unavailable out", "available out"}
                              << true;
}

void TestComponent::provideInterface()
{
    QFETCH(QList<int>, pointers);
    QFETCH(QStringList, emitted);
    QFETCH(bool, available);

    QObject first;
    QObject second;
    QObject* const objects[] = {nullptr, &first, &second};
    QStringList recorded;
    const std::unique_ptr<Probe> probe = createProbe();
    const QSignalSpy interfacesChanged(probe.get(), &Nebula::Component::interfacesChanged);
    recordInterfaceSignals(*probe, recorded);

    for(const int pointer : pointers)
    {
        QVERIFY(probe->provideObject(QStringLiteral("out"), objects[pointer]));
    }
    QCOMPARE(recorded, emitted);
    QVERIFY(interfacesChanged.count() > 0); // Declared, at least

    const QList<InterfaceInfo> provided = probe->providedInterfaces();
    QCOMPARE(provided.size(), 1);
    QCOMPARE(provided.first().name, QStringLiteral("out"));
    QCOMPARE(provided.first().type, QMetaType::fromType<QObject*>());
    QCOMPARE(provided.first().available, available);
    QCOMPARE(provided.first().requirement, InterfaceRequirement::Optional);
    QCOMPARE(probe->providedInterface<QObject>(QStringLiteral("out")), objects[pointers.last()]);
}

void TestComponent::providedInterface_data()
{
    QTest::addColumn<bool>("providesTimer");
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("asksTimer");
    QTest::addColumn<bool>("found");

    QTest::newRow("object as an object") << false << "out" << false << true;
    QTest::newRow("timer as a timer") << true << "out" << true << true;
    // The type of the pointer, exactly: not one it derives from.
    QTest::newRow("timer as an object") << true << "out" << false << false;
    QTest::newRow("object as a timer") << false << "out" << true << false;
    QTest::newRow("unknown name") << false << "other" << false << false;
}

void TestComponent::providedInterface()
{
    QFETCH(bool, providesTimer);
    QFETCH(QString, name);
    QFETCH(bool, asksTimer);
    QFETCH(bool, found);

    QTimer timer;
    const std::unique_ptr<Probe> probe = createProbe();
    QVERIFY(providesTimer ? probe->provideTimer(QStringLiteral("out"), &timer)
                          : probe->provideObject(QStringLiteral("out"), &timer));
    const void* pointer = asksTimer ? static_cast<const void*>(probe->providedInterface<QTimer>(name))
                                    : static_cast<const void*>(probe->providedInterface<QObject>(name));
    QCOMPARE(pointer != nullptr, found);
}

void TestComponent::provideWithAnotherTypeIsRefused()
{
    const std::unique_ptr<Probe> probe = createProbe();
    QVERIFY(probe->provideObject(QStringLiteral("out")));
    QTest::ignoreMessage(QtWarningMsg, "Interface out cannot be provided as QTimer* - already provided as QObject*");
    QVERIFY(!probe->provideTimer(QStringLiteral("out")));
    QCOMPARE(probe->providedInterfaces().size(), 1);
    QCOMPARE(probe->providedInterfaces().first().type, QMetaType::fromType<QObject*>());
}

void TestComponent::consumeTwiceIsRefused()
{
    const std::unique_ptr<Probe> probe = createProbe();
    QVERIFY(probe->consumeObject(QStringLiteral("in")));
    QTest::ignoreMessage(QtWarningMsg, "Interface in is already consumed");
    QVERIFY(!probe->consumeTimer(QStringLiteral("in")));
    QCOMPARE(probe->consumedInterfaces().size(), 1);
}

void TestComponent::consumedInterfaces()
{
    const std::unique_ptr<Probe> probe = createProbe();
    QVERIFY(probe->consumeObject(QStringLiteral("first")));
    QVERIFY(probe->consumeTimer(QStringLiteral("second"), InterfaceRequirement::Mandatory));

    const QList<InterfaceInfo> consumed = probe->consumedInterfaces();
    QCOMPARE(consumed.size(), 2); // In the order they were declared
    QCOMPARE(consumed.at(0).name, QStringLiteral("first"));
    QCOMPARE(consumed.at(0).type, QMetaType::fromType<QObject*>());
    QCOMPARE(consumed.at(0).requirement, InterfaceRequirement::Optional);
    QCOMPARE(consumed.at(1).name, QStringLiteral("second"));
    QCOMPARE(consumed.at(1).type, QMetaType::fromType<QTimer*>());
    QCOMPARE(consumed.at(1).requirement, InterfaceRequirement::Mandatory);
    QVERIFY(!consumed.at(0).available);
    QVERIFY(!consumed.at(1).available);
    QVERIFY(probe->providedInterfaces().isEmpty());
}

void TestComponent::bindInterface_data()
{
    QTest::addColumn<bool>("consumesTimer");
    QTest::addColumn<QString>("consumed");
    QTest::addColumn<QString>("provided");
    QTest::addColumn<bool>("providesTimer");
    QTest::addColumn<bool>("toItself");
    QTest::addColumn<bool>("bound");

    QTest::newRow("same type") << false << "in" << "out" << false << false << true;
    QTest::newRow("same type, timers") << true << "in" << "out" << true << false << true;
    QTest::newRow("another type") << false << "in" << "out" << true << false << false;
    QTest::newRow("not consumed") << false << "unknown" << "out" << false << false << false;
    QTest::newRow("not provided") << false << "in" << "unknown" << false << false << false;
    QTest::newRow("to itself") << false << "in" << "out" << false << true << false;
}

void TestComponent::bindInterface()
{
    QFETCH(bool, consumesTimer);
    QFETCH(QString, consumed);
    QFETCH(QString, provided);
    QFETCH(bool, providesTimer);
    QFETCH(bool, toItself);
    QFETCH(bool, bound);

    const std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> other = createProbe();
    Probe& provider = toItself ? *consumer : *other;
    QVERIFY(consumesTimer ? consumer->consumeTimer(QStringLiteral("in"))
                          : consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(providesTimer ? provider.provideTimer(QStringLiteral("out"))
                          : provider.provideObject(QStringLiteral("out")));

    QCOMPARE(consumer->bindInterface(consumed, provider, provided), bound);
    const Nebula::InterfaceBinding binding = consumer->binding(consumed);
    QCOMPARE(binding.provider, bound ? &provider : nullptr);
    QCOMPARE(binding.providedName, bound ? provided : QString());
}

void TestComponent::boundInterfaceFollowsItsProvider()
{
    QObject object;
    const std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> provider = createProbe();
    QVERIFY(consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(provider->provideObject(QStringLiteral("out")));

    // Bound to an interface that is not available: nothing to deliver yet.
    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *provider, QStringLiteral("out")));
    QCOMPARE(consumer->consumed(QStringLiteral("in")), nullptr);
    QVERIFY(consumer->events().isEmpty());

    QVERIFY(provider->provideObject(QStringLiteral("out"), &object));
    QCOMPARE(consumer->consumed(QStringLiteral("in")), &object);
    QVERIFY(consumer->consumedInterfaces().first().available);

    QVERIFY(provider->provideObject(QStringLiteral("out"), nullptr));
    QCOMPARE(consumer->consumed(QStringLiteral("in")), nullptr);
    QVERIFY(!consumer->consumedInterfaces().first().available);
    // Its member still points to the object when it is told: the last time it can use it.
    QCOMPARE(consumer->events(), (QStringList{"available in", "unavailable in (set)"}));
}

void TestComponent::bindingToAnAvailableInterfaceDeliversIt()
{
    QObject object;
    const std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> provider = createProbe();
    QVERIFY(consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(provider->provideObject(QStringLiteral("out"), &object));

    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *provider, QStringLiteral("out")));
    QCOMPARE(consumer->consumed(QStringLiteral("in")), &object);
    QCOMPARE(consumer->events(), QStringList{"available in"});
}

void TestComponent::bindingAgainReplacesTheProvider()
{
    QObject firstObject;
    QObject secondObject;
    const std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> first = createProbe();
    const std::unique_ptr<Probe> second = createProbe();
    QVERIFY(consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(first->provideObject(QStringLiteral("out"), &firstObject));
    QVERIFY(second->provideObject(QStringLiteral("out"), &secondObject));
    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *first, QStringLiteral("out")));

    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *second, QStringLiteral("out")));
    QCOMPARE(consumer->binding(QStringLiteral("in")).provider, second.get());
    QCOMPARE(consumer->consumed(QStringLiteral("in")), &secondObject);
    QCOMPARE(consumer->events(), (QStringList{"available in", "unavailable in (set)", "available in"}));

    // No longer bound to the first one: what it provides no longer reaches the consumer.
    consumer->clearEvents();
    QVERIFY(first->provideObject(QStringLiteral("out"), nullptr));
    QCOMPARE(consumer->consumed(QStringLiteral("in")), &secondObject);
    QVERIFY(consumer->events().isEmpty());
}

void TestComponent::unbindInterface()
{
    QObject object;
    const std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> provider = createProbe();
    QVERIFY(consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(provider->provideObject(QStringLiteral("out"), &object));
    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *provider, QStringLiteral("out")));
    const QSignalSpy interfacesChanged(consumer.get(), &Nebula::Component::interfacesChanged);

    consumer->unbindInterface(QStringLiteral("in"));
    QCOMPARE(consumer->binding(QStringLiteral("in")).provider, nullptr);
    QCOMPARE(consumer->consumed(QStringLiteral("in")), nullptr);
    QCOMPARE(consumer->events(), (QStringList{"available in", "unavailable in (set)"}));
    QVERIFY(interfacesChanged.count() > 0);

    // Unbound already, or not consumed: nothing happens.
    consumer->clearEvents();
    consumer->unbindInterface(QStringLiteral("in"));
    consumer->unbindInterface(QStringLiteral("unknown"));
    QVERIFY(consumer->events().isEmpty());
}

void TestComponent::destroyedProviderUnbinds()
{
    QObject object;
    const std::unique_ptr<Probe> consumer = createProbe();
    std::unique_ptr<Probe> provider = createProbe();
    QVERIFY(consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(provider->provideObject(QStringLiteral("out"), &object));
    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *provider, QStringLiteral("out")));

    provider.reset();
    QCOMPARE(consumer->binding(QStringLiteral("in")).provider, nullptr);
    QCOMPARE(consumer->consumed(QStringLiteral("in")), nullptr);
    QCOMPARE(consumer->events(), (QStringList{"available in", "unavailable in (set)"}));
    QCOMPARE(consumer->unboundMandatoryInterfaces(), QStringList());
}

void TestComponent::destroyedConsumerLeavesItsProvider()
{
    QObject object;
    std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> provider = createProbe();
    QVERIFY(consumer->consumeObject(QStringLiteral("in")));
    QVERIFY(provider->provideObject(QStringLiteral("out"), &object));
    QVERIFY(consumer->bindInterface(QStringLiteral("in"), *provider, QStringLiteral("out")));

    consumer.reset();
    // Nothing is delivered to it any longer.
    QVERIFY(provider->provideObject(QStringLiteral("out"), nullptr));
    QVERIFY(provider->provideObject(QStringLiteral("out"), &object));
    QVERIFY(provider->providedInterfaces().first().available);
}

void TestComponent::unboundMandatoryInterfaces_data()
{
    // For the interfaces "a" and "b" it consumes: whether it needs them, and whether they are bound.
    QTest::addColumn<QList<bool>>("mandatory");
    QTest::addColumn<QList<bool>>("bound");
    QTest::addColumn<QStringList>("unbound");

    QTest::newRow("optional, unbound") << QList<bool>{false, false} << QList<bool>{false, false} << QStringList{};
    QTest::newRow("mandatory, unbound") << QList<bool>{true, true} << QList<bool>{false, false}
                                        << QStringList{"a", "b"};
    QTest::newRow("mandatory, bound") << QList<bool>{true, true} << QList<bool>{true, true} << QStringList{};
    QTest::newRow("one of two bound") << QList<bool>{true, true} << QList<bool>{true, false} << QStringList{"b"};
    QTest::newRow("mixed") << QList<bool>{false, true} << QList<bool>{false, false} << QStringList{"b"};
}

void TestComponent::unboundMandatoryInterfaces()
{
    QFETCH(QList<bool>, mandatory);
    QFETCH(QList<bool>, bound);
    QFETCH(QStringList, unbound);

    const std::unique_ptr<Probe> consumer = createProbe();
    const std::unique_ptr<Probe> provider = createProbe();
    QVERIFY(provider->provideObject(QStringLiteral("out")));
    const QStringList names{"a", "b"};
    for(qsizetype index = 0; index < names.size(); ++index)
    {
        QVERIFY(consumer->consumeObject(names.at(index), mandatory.at(index) ? InterfaceRequirement::Mandatory
                                                                             : InterfaceRequirement::Optional));
        if(bound.at(index))
        {
            // Bound is enough, available or not.
            QVERIFY(consumer->bindInterface(names.at(index), *provider, QStringLiteral("out")));
        }
    }
    QCOMPARE(consumer->unboundMandatoryInterfaces(), unbound);
}

void TestComponent::addPropertiesForQObject()
{
    const std::unique_ptr<Probe> probe = createProbe();
    const QSignalSpy changed(probe.get(), &Nebula::Component::propertyObjectsChanged);
    auto* named = new Options(probe.get());
    auto unnamed = std::make_unique<Options>(); // Need not be a child, nor outlive it

    probe->addObject(named, QStringLiteral("group"));
    probe->addObject(unnamed.get());
    QCOMPARE(changed.count(), 2);
    QList<Nebula::PropertyObject> objects = probe->propertyObjects();
    QCOMPARE(objects.size(), 2); // In the order they were added
    QCOMPARE(objects.at(0).object, named);
    QCOMPARE(objects.at(0).name, QStringLiteral("group"));
    QCOMPARE(objects.at(1).object, unnamed.get());
    QCOMPARE(objects.at(1).name, QString());

    // It leaves once destroyed.
    delete named;
    QCOMPARE(changed.count(), 3);
    objects = probe->propertyObjects();
    QCOMPARE(objects.size(), 1);
    QCOMPARE(objects.first().object, unnamed.get());
}

void TestComponent::addPropertiesForQObjectRefuses_data()
{
    QTest::addColumn<QString>("object");

    QTest::newRow("nullptr") << "nullptr";
    QTest::newRow("the component itself") << "component";
    QTest::newRow("already added") << "added";
}

void TestComponent::addPropertiesForQObjectRefuses()
{
    QFETCH(QString, object);

    const std::unique_ptr<Probe> probe = createProbe();
    Options options;
    probe->addObject(&options);
    const QSignalSpy changed(probe.get(), &Nebula::Component::propertyObjectsChanged);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^The properties of .* cannot be added to")));
    const QHash<QString, QObject*> objects{{QStringLiteral("nullptr"), nullptr},
                                           {QStringLiteral("component"), probe.get()},
                                           {QStringLiteral("added"), &options}};
    QObject* refused = objects.value(object);
    probe->addObject(refused, QStringLiteral("again"));
    QCOMPARE(changed.count(), 0);
    QCOMPARE(probe->propertyObjects().size(), 1);
}

void TestComponent::propertyObjectProperties_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<bool>("listed");

    QTest::newRow("level") << "level" << true;
    QTest::newRow("enabled") << "enabled" << true;
    // QObject gives it to every object.
    QTest::newRow("objectName") << "objectName" << false;
    QTest::newRow("DESIGNABLE false") << "hint" << false;
    QTest::newRow("STORED false") << "cache" << false;
}

void TestComponent::propertyObjectProperties()
{
    QFETCH(QString, property);
    QFETCH(bool, listed);

    Options options;
    const Nebula::PropertyObject object{.object = &options, .name = {}};
    const QList<QMetaProperty> properties = object.properties();
    QCOMPARE(std::ranges::any_of(properties, [&property](const QMetaProperty& known)
                                 { return QLatin1StringView(known.name()) == property; }),
             listed);
    QVERIFY(Nebula::PropertyObject{}.properties().isEmpty()); // Without an object
}

QTEST_GUILESS_MAIN(TestComponent)
#include "TestComponent.moc"
