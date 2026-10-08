#include <QList>
#include <QMetaProperty>
#include <QStringList>
#include <QTest>
#include <memory>
#include <optional>

#include "Component/Component.h"
#include "Component/ComponentFactory.h"
#include "Plugin/Plugin.h"

// A base class of components, not registered: its property and its category are those of the components deriving
// from it.
class Base : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("category", "Base category")
    Q_PROPERTY(int base READ base CONSTANT)

  public:
    [[nodiscard]] int base() const
    {
        return 1;
    }
};

namespace Fixtures
{

// Has a description, a category and two properties; its name leaves its namespace out.
class Described : public AppForge::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Described component")
    Q_CLASSINFO("category", "Tests")
    Q_PROPERTY(int count READ count WRITE setCount)
    Q_PROPERTY(QString label READ label CONSTANT)

  public:
    [[nodiscard]] int count() const
    {
        return m_count;
    }

    void setCount(int count)
    {
        m_count = count;
    }

    [[nodiscard]] QString label() const
    {
        return QStringLiteral("Label");
    }

  private:
    int m_count = 0;
};

} // namespace Fixtures

// Neither description, nor category, nor property.
class Bare : public AppForge::Component
{
    Q_OBJECT
};

// Inherits the property and the category of Base.
class Derived : public Base
{
    Q_OBJECT
    Q_PROPERTY(int derived READ derived CONSTANT)

  public:
    [[nodiscard]] int derived() const
    {
        return 2;
    }
};

// Records the components it builds.
class FactoryTestPlugin : public AppForge::Plugin
{
  public:
    void build(Fixtures::Described& described)
    {
        built.append(&described);
    }

    void build(Bare& bare)
    {
        built.append(&bare);
    }

    void build(Derived& derived)
    {
        built.append(&derived);
    }

    QList<const AppForge::Component*> built;
};

APPFORGE_REGISTER_COMPONENT(FactoryTestPlugin, Fixtures::Described);
APPFORGE_REGISTER_COMPONENT(FactoryTestPlugin, Bare);
APPFORGE_REGISTER_COMPONENT(FactoryTestPlugin, Derived);

namespace
{

// Names of the properties of component.
QStringList propertyNames(const AppForge::ComponentInfo& component)
{
    QStringList names;
    for(const QMetaProperty& property : component.properties)
    {
        names.append(QString::fromLatin1(property.name()));
    }
    return names;
}

FactoryTestPlugin& builder(AppForge::Plugin& plugin)
{
    return static_cast<FactoryTestPlugin&>(plugin);
}

} // namespace

class ComponentFactoryTest : public QObject
{
    Q_OBJECT

  private slots:
    void init();

    void registersComponentsOfPlugin();
    void readsInfoFromMetaObject();
    void emptyInfoWhenNotGiven();
    void inheritsFromBaseClasses();
    void unknownComponent();
    void createHasPluginBuildIt();
    void addingPluginAgainDoesNothing();
    void skipsIdAlreadyRegistered();
    void samePluginClassUnderAnotherId();

  private:
    // A new factory for each test, where m_plugin is registered as Tests.Factory.
    std::unique_ptr<AppForge::ComponentFactory> m_factory;
    std::unique_ptr<AppForge::Plugin> m_plugin;
};

void ComponentFactoryTest::init()
{
    m_factory = std::make_unique<AppForge::ComponentFactory>();
    m_plugin = AppForge::Plugin::instantiate<FactoryTestPlugin>();
    m_factory->add(QStringLiteral("Tests.Factory"), *m_plugin);
}

void ComponentFactoryTest::registersComponentsOfPlugin()
{
    QStringList ids;
    for(const AppForge::ComponentInfo& component : m_factory->components())
    {
        ids.append(component.id);
    }
    ids.sort();
    QCOMPARE(ids, (QStringList{QStringLiteral("Tests.Factory.Bare"), QStringLiteral("Tests.Factory.Derived"),
                               QStringLiteral("Tests.Factory.Described")}));
}

void ComponentFactoryTest::readsInfoFromMetaObject()
{
    const std::optional<AppForge::ComponentInfo> described =
        m_factory->component(QStringLiteral("Tests.Factory.Described"));
    QVERIFY(described);
    QCOMPARE(described->id, QStringLiteral("Tests.Factory.Described"));
    QCOMPARE(described->name, QStringLiteral("Described"));
    QCOMPARE(described->pluginId, QStringLiteral("Tests.Factory"));
    QCOMPARE(described->description, QStringLiteral("Described component"));
    QCOMPARE(described->category, QStringLiteral("Tests"));
    // Neither objectName (QObject) nor componentId (Component).
    QCOMPARE(propertyNames(*described), (QStringList{QStringLiteral("count"), QStringLiteral("label")}));
    QVERIFY(described->properties.at(0).isWritable());
    QVERIFY(!described->properties.at(1).isWritable());
}

void ComponentFactoryTest::emptyInfoWhenNotGiven()
{
    const std::optional<AppForge::ComponentInfo> bare = m_factory->component(QStringLiteral("Tests.Factory.Bare"));
    QVERIFY(bare);
    QCOMPARE(bare->name, QStringLiteral("Bare"));
    QVERIFY(bare->description.isEmpty());
    QVERIFY(bare->category.isEmpty());
    QVERIFY(bare->properties.isEmpty());
}

void ComponentFactoryTest::inheritsFromBaseClasses()
{
    const std::optional<AppForge::ComponentInfo> derived =
        m_factory->component(QStringLiteral("Tests.Factory.Derived"));
    QVERIFY(derived);
    QCOMPARE(derived->category, QStringLiteral("Base category"));
    QVERIFY(derived->description.isEmpty());
    QCOMPARE(propertyNames(*derived), (QStringList{QStringLiteral("base"), QStringLiteral("derived")}));
}

void ComponentFactoryTest::unknownComponent()
{
    QVERIFY(!m_factory->component(QStringLiteral("Tests.Factory.Unknown")));
    QVERIFY(!m_factory->create(QStringLiteral("Tests.Factory.Unknown")));
    // A base class is not a component of the plugin.
    QVERIFY(!m_factory->component(QStringLiteral("Tests.Factory.Base")));
    // Ids are not matched by name alone.
    QVERIFY(!m_factory->component(QStringLiteral("Bare")));
}

void ComponentFactoryTest::createHasPluginBuildIt()
{
    const std::unique_ptr<AppForge::Component> component =
        m_factory->create(QStringLiteral("Tests.Factory.Described"));
    QVERIFY(component);
    QCOMPARE(builder(*m_plugin).built, (QList<const AppForge::Component*>{component.get()}));
    QVERIFY(qobject_cast<Fixtures::Described*>(component.get()));
    QCOMPARE(component->metaObject(), &Fixtures::Described::staticMetaObject);
    QCOMPARE(component->componentId(), QStringLiteral("Tests.Factory.Described"));
    QCOMPARE(component->state(), AppForge::ComponentState::Initializing);
    QVERIFY(component->setProperty("count", 3));
    QCOMPARE(component->property("count").toInt(), 3);

    // Each call builds another one.
    const std::unique_ptr<AppForge::Component> other = m_factory->create(QStringLiteral("Tests.Factory.Described"));
    QVERIFY(other);
    QVERIFY(other.get() != component.get());
    QCOMPARE(builder(*m_plugin).built.size(), 2);
}

void ComponentFactoryTest::addingPluginAgainDoesNothing()
{
    m_factory->add(QStringLiteral("Tests.Factory"), *m_plugin);
    m_factory->add(QStringLiteral("Tests.Other"), *m_plugin);
    QCOMPARE(m_factory->components().size(), 3);
    QVERIFY(!m_factory->component(QStringLiteral("Tests.Other.Bare")));
}

void ComponentFactoryTest::skipsIdAlreadyRegistered()
{
    const std::unique_ptr<AppForge::Plugin> other = AppForge::Plugin::instantiate<FactoryTestPlugin>();
    QTest::ignoreMessage(QtWarningMsg, "Ignoring Tests.Factory.Described - already registered");
    QTest::ignoreMessage(QtWarningMsg, "Ignoring Tests.Factory.Bare - already registered");
    QTest::ignoreMessage(QtWarningMsg, "Ignoring Tests.Factory.Derived - already registered");
    m_factory->add(QStringLiteral("Tests.Factory"), *other);
    QCOMPARE(m_factory->components().size(), 3);

    // The plugin registered first still builds them.
    const std::unique_ptr<AppForge::Component> bare = m_factory->create(QStringLiteral("Tests.Factory.Bare"));
    QVERIFY(bare);
    QCOMPARE(builder(*m_plugin).built.size(), 1);
    QVERIFY(builder(*other).built.isEmpty());
}

void ComponentFactoryTest::samePluginClassUnderAnotherId()
{
    const std::unique_ptr<AppForge::Plugin> other = AppForge::Plugin::instantiate<FactoryTestPlugin>();
    m_factory->add(QStringLiteral("Tests.Other"), *other);
    QCOMPARE(m_factory->components().size(), 6);

    const std::optional<AppForge::ComponentInfo> bare = m_factory->component(QStringLiteral("Tests.Other.Bare"));
    QVERIFY(bare);
    QCOMPARE(bare->pluginId, QStringLiteral("Tests.Other"));
    const std::unique_ptr<AppForge::Component> component = m_factory->create(QStringLiteral("Tests.Other.Bare"));
    QVERIFY(component);
    QCOMPARE(component->componentId(), QStringLiteral("Tests.Other.Bare"));
    QCOMPARE(builder(*other).built.size(), 1);
    QVERIFY(builder(*m_plugin).built.isEmpty());
}

QTEST_GUILESS_MAIN(ComponentFactoryTest)

#include "ComponentFactoryTest.moc"
