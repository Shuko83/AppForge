#include <QList>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
#include <memory>

#include "Component/Component.h"
#include "Component/ComponentFactory.h"
#include "Plugin/Plugin.h"

// Interfaces the components of the tests provide and consume; not abstract, the tests only compare their addresses.
class ISource
{
  public:
    virtual ~ISource() = default;
};

APPFORGE_DECLARE_INTERFACE(ISource, "Tests.ISource/1.0");

class ISink
{
  public:
    virtual ~ISink() = default;
};

APPFORGE_DECLARE_INTERFACE(ISink, "Tests.ISink/1.0");

// Counts the calls of its hooks, which accept or refuse their step as told.
class Lifecycle : public AppForge::Component
{
    Q_OBJECT

  public:
    bool acceptInitialize = true;
    bool acceptStart = true;
    int initializeCalls = 0;
    int startCalls = 0;
    int stopCalls = 0;

  protected:
    bool onInitialize() override
    {
        ++initializeCalls;
        return acceptInitialize;
    }

    bool onStart() override
    {
        ++startCalls;
        return acceptStart;
    }

    void onStop() override
    {
        ++stopCalls;
    }
};

// Provides the ISource source points to.
class Provider : public AppForge::Component
{
    Q_OBJECT

  public:
    Provider()
    {
        provideInterface<ISource>(source);
    }

    ISource* source = nullptr;
};

// Consumes an ISource and an ISink.
class Consumer : public AppForge::Component
{
    Q_OBJECT

  public:
    Consumer()
    {
        consumeInterface<ISource>(source);
        consumeInterface<ISink>(sink);
    }

    ISource* source = nullptr;
    ISink* sink = nullptr;
};

// Declares each of its interfaces twice: the second declaration is ignored.
class Duplicate : public AppForge::Component
{
    Q_OBJECT

  public:
    Duplicate()
    {
        provideInterface<ISource>(firstSource);
        provideInterface<ISource>(secondSource);
        consumeInterface<ISink>(firstSink);
        consumeInterface<ISink>(secondSink);
    }

    ISource* firstSource = nullptr;
    ISource* secondSource = nullptr;
    ISink* firstSink = nullptr;
    ISink* secondSink = nullptr;
};

// Builds the components of the tests, which hold no logic.
class ComponentTestPlugin : public AppForge::Plugin
{
  public:
    void build(Lifecycle& /*lifecycle*/) {}
    void build(Provider& /*provider*/) {}
    void build(Consumer& /*consumer*/) {}
    void build(Duplicate& /*duplicate*/) {}
};

APPFORGE_REGISTER_COMPONENT(ComponentTestPlugin, Lifecycle);
APPFORGE_REGISTER_COMPONENT(ComponentTestPlugin, Provider);
APPFORGE_REGISTER_COMPONENT(ComponentTestPlugin, Consumer);
APPFORGE_REGISTER_COMPONENT(ComponentTestPlugin, Duplicate);

using AppForge::ComponentState;
using States = QList<ComponentState>;

class ComponentTest : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();

    void createdInitializing();
    void lifecycle();
    void stepsRefusedByState();
    void stepsRefusedByHooks();

    void declaredInterfaces();
    void duplicateDeclarationsIgnored();
    void bindGivesProvidedPointer();
    void bindReadsPointerWhenBound();
    void bindRefusedChangesNothing();
    void bindAgainReplacesProvider();
    void bindWhileRunning();
    void unbind();
    void providerDestroyedUnbinds();
    void consumerDestroyedFirst();

  private:
    // Has ComponentTestPlugin build a T, as ComponentFactory does for the components of a loaded plugin.
    template <typename T> std::unique_ptr<T> create() const
    {
        const QString componentId = QStringLiteral("Tests.ComponentTest.") + T::staticMetaObject.className();
        std::unique_ptr<AppForge::Component> component = m_factory.create(componentId);
        T* typed = qobject_cast<T*>(component.get());
        if(typed != nullptr)
        {
            component.release();
        }
        return std::unique_ptr<T>(typed);
    }

    // The states component goes through from now on.
    static std::shared_ptr<States> record(const AppForge::Component& component)
    {
        auto states = std::make_shared<States>();
        connect(&component, &AppForge::Component::stateChanged,
                [states](ComponentState state) { states->append(state); });
        return states;
    }

    std::unique_ptr<AppForge::Plugin> m_plugin = AppForge::Plugin::instantiate<ComponentTestPlugin>();
    AppForge::ComponentFactory m_factory;
};

void ComponentTest::initTestCase()
{
    m_factory.add(QStringLiteral("Tests.ComponentTest"), *m_plugin);
}

void ComponentTest::createdInitializing()
{
    const std::unique_ptr<Lifecycle> lifecycle = create<Lifecycle>();
    QVERIFY(lifecycle);
    QCOMPARE(lifecycle->state(), ComponentState::Initializing);
    QCOMPARE(lifecycle->componentId(), QStringLiteral("Tests.ComponentTest.Lifecycle"));
    QCOMPARE(lifecycle->property("componentId").toString(), QStringLiteral("Tests.ComponentTest.Lifecycle"));
    QCOMPARE(lifecycle->initializeCalls, 0);
}

void ComponentTest::lifecycle()
{
    const std::unique_ptr<Lifecycle> lifecycle = create<Lifecycle>();
    const std::shared_ptr<States> states = record(*lifecycle);

    QVERIFY(lifecycle->initialize());
    QCOMPARE(lifecycle->state(), ComponentState::Ready);
    QVERIFY(lifecycle->start());
    QCOMPARE(lifecycle->state(), ComponentState::Running);
    lifecycle->stop();
    QCOMPARE(lifecycle->state(), ComponentState::Ready);
    // A stopped component starts again.
    QVERIFY(lifecycle->start());
    QCOMPARE(lifecycle->state(), ComponentState::Running);

    QCOMPARE(lifecycle->initializeCalls, 1);
    QCOMPARE(lifecycle->startCalls, 2);
    QCOMPARE(lifecycle->stopCalls, 1);
    QCOMPARE(*states,
             (States{ComponentState::Ready, ComponentState::Running, ComponentState::Ready, ComponentState::Running}));
}

void ComponentTest::stepsRefusedByState()
{
    const std::unique_ptr<Lifecycle> lifecycle = create<Lifecycle>();
    const std::shared_ptr<States> states = record(*lifecycle);

    // Initializing
    QVERIFY(!lifecycle->start());
    lifecycle->stop();
    QCOMPARE(lifecycle->state(), ComponentState::Initializing);

    // Ready
    QVERIFY(lifecycle->initialize());
    QVERIFY(!lifecycle->initialize());
    lifecycle->stop();
    QCOMPARE(lifecycle->state(), ComponentState::Ready);

    // Running
    QVERIFY(lifecycle->start());
    QVERIFY(!lifecycle->initialize());
    QVERIFY(!lifecycle->start());
    QCOMPARE(lifecycle->state(), ComponentState::Running);

    // The hooks are only called for the steps the state allows.
    QCOMPARE(lifecycle->initializeCalls, 1);
    QCOMPARE(lifecycle->startCalls, 1);
    QCOMPARE(lifecycle->stopCalls, 0);
    QCOMPARE(*states, (States{ComponentState::Ready, ComponentState::Running}));
}

void ComponentTest::stepsRefusedByHooks()
{
    const std::unique_ptr<Lifecycle> lifecycle = create<Lifecycle>();
    const std::shared_ptr<States> states = record(*lifecycle);

    lifecycle->acceptInitialize = false;
    QVERIFY(!lifecycle->initialize());
    QCOMPARE(lifecycle->state(), ComponentState::Initializing);

    lifecycle->acceptInitialize = true;
    QVERIFY(lifecycle->initialize());

    lifecycle->acceptStart = false;
    QVERIFY(!lifecycle->start());
    QCOMPARE(lifecycle->state(), ComponentState::Ready);

    QCOMPARE(lifecycle->initializeCalls, 2);
    QCOMPARE(lifecycle->startCalls, 1);
    QCOMPARE(*states, (States{ComponentState::Ready}));
}

void ComponentTest::declaredInterfaces()
{
    QCOMPARE(AppForge::interfaceId<ISource>(), QStringLiteral("Tests.ISource/1.0"));

    const std::unique_ptr<Provider> provider = create<Provider>();
    QCOMPARE(provider->providedInterfaces(), QStringList{QStringLiteral("Tests.ISource/1.0")});
    QVERIFY(provider->consumedInterfaces().isEmpty());

    // In the order they were declared.
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    QCOMPARE(consumer->consumedInterfaces(),
             (QStringList{QStringLiteral("Tests.ISource/1.0"), QStringLiteral("Tests.ISink/1.0")}));
    QVERIFY(consumer->providedInterfaces().isEmpty());
}

void ComponentTest::duplicateDeclarationsIgnored()
{
    QTest::ignoreMessage(QtWarningMsg, "Duplicate already provides Tests.ISource/1.0");
    QTest::ignoreMessage(QtWarningMsg, "Duplicate already consumes Tests.ISink/1.0");
    const std::unique_ptr<Duplicate> duplicate = create<Duplicate>();
    QCOMPARE(duplicate->providedInterfaces(), QStringList{QStringLiteral("Tests.ISource/1.0")});
    QCOMPARE(duplicate->consumedInterfaces(), QStringList{QStringLiteral("Tests.ISink/1.0")});

    // The members of the first declarations are the ones bound.
    ISource source;
    duplicate->firstSource = &source;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    QVERIFY(consumer->bindInterface<ISource>(*duplicate));
    QCOMPARE(consumer->source, &source);
}

void ComponentTest::bindGivesProvidedPointer()
{
    ISource source;
    const std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &source;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    QCOMPARE(consumer->source, nullptr);
    QSignalSpy changed(consumer.get(), &AppForge::Component::consumedInterfaceChanged);

    QVERIFY(consumer->bindInterface<ISource>(*provider));
    QCOMPARE(consumer->source, &source);
    QCOMPARE(consumer->sink, nullptr);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.at(0).at(0).toString(), QStringLiteral("Tests.ISource/1.0"));

    // By id, without the header of the interface.
    consumer->unbindInterface(QStringLiteral("Tests.ISource/1.0"));
    QVERIFY(consumer->bindInterface(QStringLiteral("Tests.ISource/1.0"), *provider));
    QCOMPARE(consumer->source, &source);
}

void ComponentTest::bindReadsPointerWhenBound()
{
    ISource first;
    ISource second;
    const std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &first;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();

    QVERIFY(consumer->bindInterface<ISource>(*provider));
    provider->source = &second;
    QCOMPARE(consumer->source, &first);

    QVERIFY(consumer->bindInterface<ISource>(*provider));
    QCOMPARE(consumer->source, &second);
}

void ComponentTest::bindRefusedChangesNothing()
{
    ISource source;
    const std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &source;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    const std::unique_ptr<Lifecycle> lifecycle = create<Lifecycle>();
    QVERIFY(consumer->bindInterface<ISource>(*provider));
    QSignalSpy changed(consumer.get(), &AppForge::Component::consumedInterfaceChanged);

    // The provider does not provide it.
    QVERIFY(!consumer->bindInterface<ISink>(*provider));
    QVERIFY(!consumer->bindInterface<ISource>(*lifecycle));
    // The consumer does not consume it.
    QVERIFY(!provider->bindInterface<ISource>(*provider));
    QVERIFY(!consumer->bindInterface(QStringLiteral("Tests.Unknown/1.0"), *provider));

    QCOMPARE(consumer->source, &source);
    QCOMPARE(consumer->sink, nullptr);
    QCOMPARE(changed.size(), 0);
}

void ComponentTest::bindAgainReplacesProvider()
{
    ISource first;
    ISource second;
    std::unique_ptr<Provider> firstProvider = create<Provider>();
    firstProvider->source = &first;
    const std::unique_ptr<Provider> secondProvider = create<Provider>();
    secondProvider->source = &second;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    QSignalSpy changed(consumer.get(), &AppForge::Component::consumedInterfaceChanged);

    QVERIFY(consumer->bindInterface<ISource>(*firstProvider));
    QVERIFY(consumer->bindInterface<ISource>(*secondProvider));
    QCOMPARE(consumer->source, &second);
    QCOMPARE(changed.size(), 2);

    // The replaced provider no longer unbinds it.
    firstProvider.reset();
    QCOMPARE(consumer->source, &second);
    QCOMPARE(changed.size(), 2);
}

void ComponentTest::bindWhileRunning()
{
    ISource source;
    const std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &source;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    QVERIFY(consumer->initialize());
    QVERIFY(consumer->start());
    QSignalSpy changed(consumer.get(), &AppForge::Component::consumedInterfaceChanged);

    QVERIFY(consumer->bindInterface<ISource>(*provider));
    QCOMPARE(consumer->source, &source);
    consumer->unbindInterface(QStringLiteral("Tests.ISource/1.0"));
    QCOMPARE(consumer->source, nullptr);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(consumer->state(), ComponentState::Running);
}

void ComponentTest::unbind()
{
    ISource source;
    const std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &source;
    const std::unique_ptr<Consumer> consumer = create<Consumer>();
    QVERIFY(consumer->bindInterface<ISource>(*provider));
    QSignalSpy changed(consumer.get(), &AppForge::Component::consumedInterfaceChanged);

    consumer->unbindInterface(QStringLiteral("Tests.ISource/1.0"));
    QCOMPARE(consumer->source, nullptr);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.at(0).at(0).toString(), QStringLiteral("Tests.ISource/1.0"));

    // Already unbound, never bound, not consumed: nothing happens.
    consumer->unbindInterface(QStringLiteral("Tests.ISource/1.0"));
    consumer->unbindInterface(QStringLiteral("Tests.ISink/1.0"));
    consumer->unbindInterface(QStringLiteral("Tests.Unknown/1.0"));
    QCOMPARE(changed.size(), 1);
}

void ComponentTest::providerDestroyedUnbinds()
{
    ISource source;
    std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &source;
    const std::unique_ptr<Consumer> first = create<Consumer>();
    const std::unique_ptr<Consumer> second = create<Consumer>();
    QVERIFY(first->bindInterface<ISource>(*provider));
    QVERIFY(second->bindInterface<ISource>(*provider));
    QSignalSpy firstChanged(first.get(), &AppForge::Component::consumedInterfaceChanged);
    QSignalSpy secondChanged(second.get(), &AppForge::Component::consumedInterfaceChanged);

    provider.reset();
    QCOMPARE(first->source, nullptr);
    QCOMPARE(second->source, nullptr);
    QCOMPARE(firstChanged.size(), 1);
    QCOMPARE(secondChanged.size(), 1);
    QCOMPARE(firstChanged.at(0).at(0).toString(), QStringLiteral("Tests.ISource/1.0"));
}

void ComponentTest::consumerDestroyedFirst()
{
    ISource source;
    std::unique_ptr<Provider> provider = create<Provider>();
    provider->source = &source;
    std::unique_ptr<Consumer> consumer = create<Consumer>();
    QVERIFY(consumer->bindInterface<ISource>(*provider));

    // The provider must not reach its destroyed consumer.
    consumer.reset();
    provider.reset();
}

QTEST_GUILESS_MAIN(ComponentTest)

#include "ComponentTest.moc"
