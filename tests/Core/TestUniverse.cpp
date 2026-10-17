// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>
#include <cstddef>
#include <utility>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Options.h"
#include "Panel.h"
#include "Probe.h"
#include "Sink.h"
#include "Source.h"
#include "TestComponents.h"

using Nebula::Universe;

namespace
{

const QString ExamplePluginId = QStringLiteral("FabienDG.Nebula.ExamplePlugin");
const QString GreeterId = ExamplePluginId + QStringLiteral(".Greeter");

// An instance of the component of the tests T in universe, named name, or after its component when empty.
template <typename T> T* instantiate(Universe& universe, const QString& name = {})
{
    return qobject_cast<T*>(
        universe.instantiate(testComponentId(QLatin1StringView(T::staticMetaObject.className())), name));
}

// The project JSON text holds.
QJsonObject project(const QByteArray& text)
{
    return QJsonDocument::fromJson(text).object();
}

// The names of the components of universe, in their order.
QStringList names(const Universe& universe)
{
    QStringList names;
    for(const std::unique_ptr<Nebula::Component>& component : universe.components())
    {
        names.append(component->objectName());
    }
    return names;
}

} // namespace

// Universe: how it assembles an application, saves and loads it as a project, runs it, and unloads its plugins.
class TestUniverse : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();

    void instantiate_data();
    void instantiate();
    void instantiateUnknown();
    void rename_data();
    void rename();
    void remove();
    void removeUnbindsWhatItProvides();
    void componentsOf();

    void read_data();
    void read();
    void saveAndLoad_data();
    void saveAndLoad();
    void saveLeavesWhatJsonCannotHold();
    void saveAndLoadBindings();
    void saveAndLoadObjects();
    void loadReportsProblems_data();
    void loadReportsProblems();
    void loadReplacesTheComponents();
    void open();

    void startAndStop();
    void startRefused();
    void runningRestoresEditedValues_data();
    void runningRestoresEditedValues();
    void assemblyRefusedWhileRunning();
    void clear();

    void addSettingsFile_data();
    void addSettingsFile();
    void saveAndLoadSettingsFiles();

    void savePluginsRelativeToTheRoot_data();
    void savePluginsRelativeToTheRoot();
    void applyCommandLine();
    void unloadPlugin();
};

void TestUniverse::initTestCase()
{
    registerTestComponents();
    // A settings file without directory is written there.
    QStandardPaths::setTestModeEnabled(true);
}

void TestUniverse::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestUniverse::instantiate_data()
{
    QTest::addColumn<QStringList>("existing");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("named");

    QTest::newRow("first") << QStringList{} << "" << "Probe1";
    QTest::newRow("second") << QStringList{"Probe1"} << "" << "Probe2";
    QTest::newRow("first free number") << QStringList{"Probe2"} << "" << "Probe1";
    QTest::newRow("given") << QStringList{} << "Mine" << "Mine";
    QTest::newRow("given, taken") << QStringList{"Mine"} << "Mine" << "Probe1";
}

void TestUniverse::instantiate()
{
    QFETCH(QStringList, existing);
    QFETCH(QString, name);
    QFETCH(QString, named);

    Universe universe;
    for(const QString& existingName : existing)
    {
        QVERIFY(::instantiate<Probe>(universe, existingName));
    }
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);

    const Probe* probe = ::instantiate<Probe>(universe, name);
    QVERIFY(probe != nullptr);
    QCOMPARE(probe->objectName(), named);
    QCOMPARE(probe->state(), Nebula::ComponentState::Editing);
    QCOMPARE(componentsChanged.count(), 1);
    QCOMPARE(universe.component(named), probe);
    QCOMPARE(names(universe), existing + QStringList{named}); // In the order they were instantiated
}

void TestUniverse::instantiateUnknown()
{
    Universe universe;
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);
    QCOMPARE(universe.instantiate(testComponentId(QStringLiteral("Unknown"))), nullptr);
    QCOMPARE(componentsChanged.count(), 0);
    QVERIFY(universe.components().empty());
}

void TestUniverse::rename_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("renamed");

    QTest::newRow("new name") << "Renamed" << true;
    QTest::newRow("its own name") << "Probe1" << true;
    QTest::newRow("name of another") << "Probe2" << false;
    QTest::newRow("empty") << "" << false;
}

void TestUniverse::rename()
{
    QFETCH(QString, name);
    QFETCH(bool, renamed);

    Universe universe;
    Probe* first = ::instantiate<Probe>(universe);
    ::instantiate<Probe>(universe);

    QCOMPARE(universe.rename(*first, name), renamed);
    QCOMPARE(first->objectName(), renamed ? name : QStringLiteral("Probe1"));
    QCOMPARE(universe.component(first->objectName()), first);
}

void TestUniverse::remove()
{
    Universe universe;
    Universe other;
    Probe* probe = ::instantiate<Probe>(universe);
    Probe* foreign = ::instantiate<Probe>(other);
    const QPointer<Probe> removed(probe);
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);

    QVERIFY(!universe.remove(*foreign));
    QCOMPARE(componentsChanged.count(), 0);
    QVERIFY(universe.remove(*probe));
    QCOMPARE(componentsChanged.count(), 1);
    QVERIFY(removed.isNull()); // Destroyed
    QVERIFY(universe.components().empty());
    QCOMPARE(universe.component(QStringLiteral("Probe1")), nullptr);
}

void TestUniverse::removeUnbindsWhatItProvides()
{
    Universe universe;
    Source* source = ::instantiate<Source>(universe);
    Sink* sink = ::instantiate<Sink>(universe);
    QVERIFY(sink->bindInterface(QStringLiteral("object"), *source, QStringLiteral("object")));
    QVERIFY(sink->unboundMandatoryInterfaces().isEmpty());

    QVERIFY(universe.remove(*source));
    QCOMPARE(sink->binding(QStringLiteral("object")).provider, nullptr);
    QCOMPARE(sink->unboundMandatoryInterfaces(), QStringList{"object"});
}

void TestUniverse::componentsOf()
{
    Universe universe;
    Probe* probe = ::instantiate<Probe>(universe);
    Panel* panel = ::instantiate<Panel>(universe);
    QCOMPARE(universe.componentsOf(TestComponents::Id), (QList<Nebula::Component*>{probe, panel}));
    QVERIFY(universe.componentsOf(QStringLiteral("Unknown")).isEmpty());
}

void TestUniverse::read_data()
{
    QTest::addColumn<QByteArray>("content"); // Of the file; none when null
    QTest::addColumn<bool>("read");
    QTest::addColumn<QString>("error"); // Part of it

    QTest::newRow("project") << QByteArray(R"({"version": 1, "components": []})") << true << "";
    QTest::newRow("missing file") << QByteArray() << false << "";
    QTest::newRow("not JSON") << QByteArray("{ version") << false << "";
    QTest::newRow("another version") << QByteArray(R"({"version": 2})") << false
                                     << "It is not a Nebula project of version 1";
    QTest::newRow("no version") << QByteArray("{}") << false << "It is not a Nebula project of version 1";
    QTest::newRow("array") << QByteArray("[1]") << false << "It is not a Nebula project of version 1";
}

void TestUniverse::read()
{
    QFETCH(QByteArray, content);
    QFETCH(bool, read);
    QFETCH(QString, error);

    const QTemporaryDir directory;
    const QString filePath = directory.filePath(QStringLiteral("project.nbp"));
    if(!content.isNull())
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    QString why;
    const std::optional<QJsonObject> project = Universe::read(filePath, why);
    QCOMPARE(project.has_value(), read);
    QCOMPARE(why.isEmpty(), read);
    QVERIFY2(why.contains(error), qPrintable(why));
}

void TestUniverse::saveAndLoad_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QVariant>("value");

    QTest::newRow("bool") << "refusesToStart" << QVariant(true);
    QTest::newRow("int") << "count" << QVariant(-42);
    QTest::newRow("double") << "ratio" << QVariant(0.125);
    QTest::newRow("QString") << "text" << QVariant(QStringLiteral("Hello, «Nebula»"));
    QTest::newRow("QDate") << "date" << QVariant(QDate(2026, 10, 17));
    QTest::newRow("QTime") << "time" << QVariant(QTime(23, 59, 30));
    QTest::newRow("QDateTime") << "moment"
                               << QVariant(QDateTime(QDate(2026, 10, 17), QTime(8, 15, 0, 250), QTimeZone::UTC));
}

void TestUniverse::saveAndLoad()
{
    QFETCH(QString, property);
    QFETCH(QVariant, value);

    QJsonObject saved;
    {
        Universe universe;
        Probe* probe = ::instantiate<Probe>(universe, QStringLiteral("Probe"));
        QVERIFY(probe->setProperty(qPrintable(property), value));
        saved = universe.save();
    }
    const QJsonObject component = saved.value(u"components").toArray().first().toObject();
    QCOMPARE(component.value(u"name").toString(), QStringLiteral("Probe"));
    QCOMPARE(component.value(u"type").toString(), testComponentId(QStringLiteral("Probe")));
    QVERIFY(component.value(u"properties").toObject().contains(property));

    Universe universe;
    QCOMPARE(universe.load(saved), QStringList());
    const Nebula::Component* probe = universe.component(QStringLiteral("Probe"));
    QVERIFY(probe != nullptr);
    QCOMPARE(probe->property(qPrintable(property)), value);
}

void TestUniverse::saveLeavesWhatJsonCannotHold()
{
    Universe universe;
    Probe* probe = ::instantiate<Probe>(universe);
    probe->setProperty("point", QPoint(1, 2));
    QVERIFY(probe->start()); // starts is 1 now

    const QJsonObject saved = universe.save();
    QCOMPARE(saved.value(u"version").toInt(), 1);
    QVERIFY(!saved.contains(u"settings")); // Only when it has some
    const QJsonObject properties =
        saved.value(u"components").toArray().first().toObject().value(u"properties").toObject();
    QVERIFY(!properties.contains(u"point"));  // JSON has nothing for it
    QVERIFY(!properties.contains(u"starts")); // Read only
    QVERIFY(!properties.contains(u"componentId"));
    QVERIFY(!properties.contains(u"objectName")); // The name of the instance
    QVERIFY(properties.contains(u"count"));
}

void TestUniverse::saveAndLoadBindings()
{
    QJsonObject saved;
    {
        Universe universe;
        Source* source = ::instantiate<Source>(universe);
        Sink* sink = ::instantiate<Sink>(universe);
        ::instantiate<Sink>(universe); // Bound to nothing
        QVERIFY(sink->bindInterface(QStringLiteral("object"), *source, QStringLiteral("object")));
        saved = universe.save();
    }
    const QJsonArray bindings = saved.value(u"bindings").toArray();
    QCOMPARE(bindings.size(), 1);
    QCOMPARE(bindings.first().toObject(), project(R"({"consumer": "Sink1", "consumed": "object",
                                                      "provider": "Source1", "provided": "object"})"));

    Universe universe;
    QCOMPARE(universe.load(saved), QStringList());
    const Nebula::Component* sink = universe.component(QStringLiteral("Sink1"));
    QCOMPARE(sink->binding(QStringLiteral("object")).provider, universe.component(QStringLiteral("Source1")));
    QCOMPARE(universe.component(QStringLiteral("Sink2"))->binding(QStringLiteral("object")).provider, nullptr);
}

void TestUniverse::saveAndLoadObjects()
{
    QJsonObject saved;
    {
        Universe universe;
        Panel* panel = ::instantiate<Panel>(universe);
        panel->general().setProperty("level", 3);
        panel->pages().at(1)->setProperty("level", 7);
        panel->pages().at(1)->setProperty("enabled", true);
        saved = universe.save();
    }
    const QJsonArray objects = saved.value(u"components").toArray().first().toObject().value(u"objects").toArray();
    QCOMPARE(objects.size(), 3);
    QCOMPARE(objects.at(0).toObject().value(u"name").toString(), QString());
    QCOMPARE(objects.at(1).toObject().value(u"name").toString(), QStringLiteral("page"));
    QCOMPARE(objects.at(2).toObject().value(u"name").toString(), QStringLiteral("page"));
    // Neither DESIGNABLE false nor STORED false.
    QCOMPARE(objects.at(0).toObject().value(u"properties").toObject().keys(), (QStringList{"enabled", "level"}));

    Universe universe;
    QCOMPARE(universe.load(saved), QStringList());
    const auto* panel = qobject_cast<Panel*>(universe.component(QStringLiteral("Panel1")));
    QCOMPARE(panel->general().property("level").toInt(), 3);
    // The objects of a name, by their order.
    QCOMPARE(panel->pages().at(0)->property("level").toInt(), 0);
    QCOMPARE(panel->pages().at(1)->property("level").toInt(), 7);
    QCOMPARE(panel->pages().at(1)->property("enabled").toBool(), true);
}

void TestUniverse::loadReportsProblems_data()
{
    QTest::addColumn<QByteArray>("project");
    QTest::addColumn<QStringList>("problems"); // The beginning of each of them

    QTest::newRow("none") << QByteArray(R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe"}]})")
                          << QStringList{};
    QTest::newRow("unknown plugin") << QByteArray(R"({"plugins": [{"id": "Some.Plugin", "file": "none.nbdl"}]})")
                                    << QStringList{"The plugin Some.Plugin is not loaded: "};
    QTest::newRow("unknown component") << QByteArray(R"({"components": [{"name": "X", "type": "Test.Unknown"}]})")
                                       << QStringList{"X is missing: no loaded plugin provides Test.Unknown"};
    QTest::newRow("unknown property")
        << QByteArray(R"({"components": [{"name": "P", "type": "Test.Probe", "properties": {"nope": 1}}]})")
        << QStringList{"The property nope of P could not be set"};
    QTest::newRow("read-only property")
        << QByteArray(R"({"components": [{"name": "P", "type": "Test.Probe", "properties": {"starts": 1}}]})")
        << QStringList{"The property starts of P could not be set"};
    QTest::newRow("missing object")
        << QByteArray(R"({"components": [{"name": "P", "type": "Test.Panel", "objects": [{"name": "other"}]}]})")
        << QStringList{"The object other of P is missing"};
    QTest::newRow("one object too many") << QByteArray(R"({"components": [{"name": "P", "type": "Test.Panel",
        "objects": [{"name": "page"}, {"name": "page"}, {"name": "page"}]}]})")
                                         << QStringList{"The object page of P is missing"};
    QTest::newRow("property of an object") << QByteArray(R"({"components": [{"name": "P", "type": "Test.Panel",
        "objects": [{"name": "page", "properties": {"hint": "no"}}]}]})")
                                           << QStringList{"The property hint of the object page of P could not be set"};
    QTest::newRow("binding without components")
        << QByteArray(
               R"({"bindings": [{"consumer": "A", "consumed": "object", "provider": "B", "provided": "object"}]})")
        << QStringList{"object of A could not be bound to object of B"};
    QTest::newRow("binding of another type") << QByteArray(R"({"components": [{"name": "A", "type": "Test.Sink"},
        {"name": "B", "type": "Test.Source"}],
        "bindings": [{"consumer": "A", "consumed": "timer", "provider": "B", "provided": "object"}]})")
                                             << QStringList{"timer of A could not be bound to object of B"};
    QTest::newRow("settings of a missing component")
        << QByteArray(R"({"settings": [{"file": "s.json", "entries": [{"component": "Nope", "property": "count"}]}]})")
        << QStringList{"The property count of Nope is not held by s.json"};
    QTest::newRow("settings of a missing object") << QByteArray(R"({"components": [{"name": "P", "type": "Test.Panel"}],
        "settings": [{"file": "s.json", "entries": [{"component": "P", "object": "page", "index": 2, "property": "level"}]}]})")
                                                  << QStringList{"The property level of the object page of P is not "
                                                                 "held by s.json"};
    QTest::newRow("settings file name taken")
        << QByteArray(R"({"settings": [{"file": "s.json"}, {"file": "S.json"}]})")
        << QStringList{"The settings file S.json is named settings.json: another one has its name"};
}

void TestUniverse::loadReportsProblems()
{
    QFETCH(QByteArray, project);
    QFETCH(QStringList, problems);

    Universe universe;
    const QStringList reported = universe.load(::project(project));
    QCOMPARE(reported.size(), problems.size());
    for(qsizetype index = 0; index < problems.size(); ++index)
    {
        QVERIFY2(reported.at(index).startsWith(problems.at(index)), qPrintable(reported.at(index)));
    }
}

void TestUniverse::loadReplacesTheComponents()
{
    Universe universe;
    ::instantiate<Probe>(universe, QStringLiteral("Before"));
    universe.addSettingsFile();

    QCOMPARE(universe.load(project(R"({"components": [{"name": "After", "type": "Test.Probe"}]})")), QStringList());
    QCOMPARE(names(universe), QStringList{"After"});
    QVERIFY(universe.settingsFiles().empty());
}

void TestUniverse::open()
{
    const QTemporaryDir directory;
    const QString complete = directory.filePath(QStringLiteral("complete.nbp"));
    const QString partial = directory.filePath(QStringLiteral("partial.nbp"));
    for(const auto& [filePath, content] :
        {std::pair{complete, QByteArray(R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe"}]})")},
         std::pair{partial, QByteArray(R"({"version": 1, "components": [{"name": "P", "type": "Test.Probe"},
                                                                          {"name": "X", "type": "Test.Unknown"}]})")}})
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    Universe universe;
    QVERIFY(universe.open(complete));
    QVERIFY(universe.errorString().isEmpty());
    QCOMPARE(names(universe), QStringList{"P"});

    // Strict: what could be restored is, but it fails.
    QVERIFY(!universe.open(partial));
    QCOMPARE(universe.errorString(), QStringLiteral("X is missing: no loaded plugin provides Test.Unknown"));
    QCOMPARE(names(universe), QStringList{"P"});

    // Nothing is changed when the file cannot be read.
    QVERIFY(!universe.open(directory.filePath(QStringLiteral("missing.nbp"))));
    QVERIFY(!universe.errorString().isEmpty());
    QCOMPARE(names(universe), QStringList{"P"});
}

void TestUniverse::startAndStop()
{
    Universe universe;
    const QList<Probe*> probes{::instantiate<Probe>(universe), ::instantiate<Probe>(universe),
                               ::instantiate<Probe>(universe)};
    QStringList order;
    for(Probe* probe : probes)
    {
        connect(probe, &Nebula::Component::stateChanged, this, [&order, probe](Nebula::ComponentState state)
                { order.append((state == Nebula::ComponentState::Running ? u'+' : u'-') + probe->objectName()); });
    }
    const QSignalSpy runningChanged(&universe, &Universe::runningChanged);

    QVERIFY(universe.start());
    QVERIFY(universe.isRunning());
    QVERIFY(universe.errorString().isEmpty());
    QCOMPARE(runningChanged.count(), 1);
    QVERIFY(universe.start()); // Does nothing while it runs
    QCOMPARE(runningChanged.count(), 1);

    universe.stop();
    QVERIFY(!universe.isRunning());
    QCOMPARE(runningChanged.count(), 2);
    universe.stop(); // Does nothing while it does not run
    QCOMPARE(runningChanged.count(), 2);
    // In the order they were instantiated, then the other way round.
    QCOMPARE(order, (QStringList{"+Probe1", "+Probe2", "+Probe3", "-Probe3", "-Probe2", "-Probe1"}));
}

void TestUniverse::startRefused()
{
    Universe universe;
    Probe* first = ::instantiate<Probe>(universe);
    Probe* refusing = ::instantiate<Probe>(universe);
    Probe* last = ::instantiate<Probe>(universe);
    refusing->setProperty("refusesToStart", true);
    const QSignalSpy runningChanged(&universe, &Universe::runningChanged);

    QVERIFY(!universe.start());
    QCOMPARE(universe.errorString(), QStringLiteral("Probe2 could not be started"));
    QVERIFY(!universe.isRunning());
    QCOMPARE(runningChanged.count(), 0);
    // What it started is stopped again; what follows is not started.
    QCOMPARE(first->events(), (QStringList{"start", "stop"}));
    QCOMPARE(first->state(), Nebula::ComponentState::Editing);
    QCOMPARE(refusing->events(), QStringList{"start"});
    QVERIFY(last->events().isEmpty());
}

void TestUniverse::runningRestoresEditedValues_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QVariant>("edited");
    QTest::addColumn<QVariant>("changed"); // While it runs

    QTest::newRow("int") << "count" << QVariant(1) << QVariant(2);
    QTest::newRow("QString") << "text" << QVariant(QStringLiteral("edited")) << QVariant(QStringLiteral("changed"));
    QTest::newRow("QDate") << "date" << QVariant(QDate(2026, 1, 1)) << QVariant(QDate(2027, 1, 1));
}

void TestUniverse::runningRestoresEditedValues()
{
    QFETCH(QString, property);
    QFETCH(QVariant, edited);
    QFETCH(QVariant, changed);

    Universe universe;
    Probe* probe = ::instantiate<Probe>(universe);
    Panel* panel = ::instantiate<Panel>(universe);
    probe->setProperty(qPrintable(property), edited);
    panel->pages().at(1)->setProperty("level", 5);
    QVERIFY(universe.start());

    probe->setProperty(qPrintable(property), changed);
    panel->pages().at(1)->setProperty("level", 6);
    // A project saved while it runs holds the values of the edition.
    const QJsonObject saved = universe.save();
    const QJsonArray components = saved.value(u"components").toArray();
    QCOMPARE(components.at(0).toObject().value(u"properties").toObject().value(property),
             QJsonValue::fromVariant(edited));
    QCOMPARE(components.at(1)
                 .toObject()
                 .value(u"objects")
                 .toArray()
                 .at(2)
                 .toObject()
                 .value(u"properties")
                 .toObject()
                 .value(u"level")
                 .toInt(),
             5);

    universe.stop();
    QCOMPARE(probe->property(qPrintable(property)), edited);
    QCOMPARE(panel->pages().at(1)->property("level").toInt(), 5);
    QCOMPARE(probe->starts(), 1); // Read only: not restored
}

void TestUniverse::assemblyRefusedWhileRunning()
{
    Universe universe;
    Probe* probe = ::instantiate<Probe>(universe);
    QVERIFY(universe.start());

    QCOMPARE(::instantiate<Probe>(universe), nullptr);
    QVERIFY(!universe.rename(*probe, QStringLiteral("Renamed")));
    QVERIFY(!universe.remove(*probe));
    QVERIFY(!universe.unloadPlugin(TestComponents::Id));
    QCOMPARE(universe.errorString(), QStringLiteral("A plugin cannot be unloaded while the application runs"));
    QCOMPARE(names(universe), QStringList{"Probe1"});

    universe.stop();
    QVERIFY(universe.rename(*probe, QStringLiteral("Renamed")));
}

void TestUniverse::clear()
{
    Universe universe;
    const QPointer<Probe> probe = ::instantiate<Probe>(universe);
    universe.addSettingsFile();
    QVERIFY(universe.start());
    const QSignalSpy componentsChanged(&universe, &Universe::componentsChanged);
    const QSignalSpy settingsFilesChanged(&universe, &Universe::settingsFilesChanged);

    universe.clear();
    QVERIFY(!universe.isRunning());
    QVERIFY(probe.isNull());
    QVERIFY(universe.components().empty());
    QVERIFY(universe.settingsFiles().empty());
    QCOMPARE(componentsChanged.count(), 1);
    QCOMPARE(settingsFilesChanged.count(), 1);
}

void TestUniverse::addSettingsFile_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<QStringList>("fileNames");

    QTest::newRow("one") << 1 << QStringList{"settings.json"};
    QTest::newRow("three") << 3 << QStringList{"settings.json", "settings2.json", "settings3.json"};
}

void TestUniverse::addSettingsFile()
{
    QFETCH(int, count);
    QFETCH(QStringList, fileNames);

    Universe universe;
    const QSignalSpy settingsFilesChanged(&universe, &Universe::settingsFilesChanged);
    for(int index = 0; index < count; ++index)
    {
        universe.addSettingsFile();
    }
    QCOMPARE(settingsFilesChanged.count(), count);
    QStringList added;
    for(const std::unique_ptr<Nebula::SettingsFile>& file : universe.settingsFiles())
    {
        added.append(file->fileName());
        QVERIFY(file->directory().isEmpty());
        QVERIFY(file->entries().isEmpty());
        // Found by its name, whatever its case.
        QCOMPARE(universe.settingsFile(file->fileName().toUpper()), file.get());
    }
    QCOMPARE(added, fileNames);

    // The first free name.
    universe.removeSettingsFile(*universe.settingsFiles().front());
    QCOMPARE(universe.addSettingsFile().fileName(), QStringLiteral("settings.json"));
}

void TestUniverse::saveAndLoadSettingsFiles()
{
    QJsonObject saved;
    {
        Universe universe;
        Probe* probe = ::instantiate<Probe>(universe);
        Panel* panel = ::instantiate<Panel>(universe);
        Nebula::SettingsFile& file = universe.addSettingsFile();
        file.setDirectory(QStringLiteral("config"));
        QVERIFY(file.add({.component = probe, .object = probe, .property = QStringLiteral("count")}));
        QVERIFY(file.add({.component = panel, .object = panel->pages().at(1), .property = QStringLiteral("level")}));
        QVERIFY(file.add({.component = panel, .object = &panel->general(), .property = QStringLiteral("enabled")}));
        universe.addSettingsFile().setFileName(QStringLiteral("other.json"));
        saved = universe.save();
    }
    const QJsonArray settings = saved.value(u"settings").toArray();
    QCOMPARE(settings.size(), 2);
    QCOMPARE(settings.at(0).toObject(), project(R"({"file": "settings.json", "directory": "config", "entries": [
        {"component": "Probe1", "property": "count"},
        {"component": "Panel1", "object": "page", "index": 1, "property": "level"},
        {"component": "Panel1", "object": "", "property": "enabled"}]})"));
    QCOMPARE(settings.at(1).toObject(), project(R"({"file": "other.json", "directory": "", "entries": []})"));

    Universe universe;
    QCOMPARE(universe.load(saved), QStringList());
    QCOMPARE(universe.settingsFiles().size(), std::size_t{2});
    const Nebula::SettingsFile& file = *universe.settingsFiles().front();
    QCOMPARE(file.directory(), QStringLiteral("config"));
    const auto* panel = qobject_cast<Panel*>(universe.component(QStringLiteral("Panel1")));
    QCOMPARE(file.entries().size(), 3);
    QCOMPARE(file.entries().at(1).object, panel->pages().at(1));
    QCOMPARE(file.entries().at(2).object, &panel->general());
    QCOMPARE(universe.settingsFiles().back()->fileName(), QStringLiteral("other.json"));
}

void TestUniverse::savePluginsRelativeToTheRoot_data()
{
    QTest::addColumn<QString>("root");
    QTest::addColumn<QString>("file");

    QTest::newRow("its directory") << NEBULA_EXAMPLE_PLUGINS_DIR << "ExamplePlugin.nbdl";
    QTest::newRow("above it") << NEBULA_EXAMPLE_PLUGINS_DIR "/.." << "plugins/ExamplePlugin.nbdl";
}

void TestUniverse::savePluginsRelativeToTheRoot()
{
    QFETCH(QString, root);
    QFETCH(QString, file);

    QJsonObject saved;
    {
        Universe universe;
        universe.setPluginRoot(root);
        universe.plugins().scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
        QVERIFY(universe.plugins().load(ExamplePluginId));
        QVERIFY(universe.instantiate(GreeterId) != nullptr);
        ::instantiate<Probe>(universe); // Of no plugin it knows: none is written for it
        saved = universe.save();
    }
    const QJsonArray plugins = saved.value(u"plugins").toArray();
    QCOMPARE(plugins.size(), 1);
    QCOMPARE(plugins.first().toObject().value(u"id").toString(), ExamplePluginId);
    QCOMPARE(plugins.first().toObject().value(u"file").toString(), file);

    // Its plugin found again from the root, by its file.
    Universe universe;
    universe.setPluginRoot(root);
    QCOMPARE(universe.load(saved), QStringList());
    QVERIFY(universe.plugins().isLoaded(ExamplePluginId));
    QVERIFY(universe.component(QStringLiteral("Greeter1")) != nullptr);
}

void TestUniverse::applyCommandLine()
{
    const QTemporaryDir root;
    const QString missing = root.filePath(QStringLiteral("missing"));
    QCommandLineParser parser;
    Universe::addCommandLineOptions(parser);
    QVERIFY(
        parser.parse({QStringLiteral("app"), QStringLiteral("--plugin-dir"), missing, QStringLiteral("--plugin-dir"),
                      QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR), QStringLiteral("--plugin-root"), root.path()}));

    Universe universe;
    QTest::ignoreMessage(
        QtWarningMsg,
        qPrintable(QStringLiteral("Skipping %1 - it is not a directory").arg(QDir::toNativeSeparators(missing))));
    universe.applyCommandLine(parser);
    QVERIFY(universe.plugins().plugin(ExamplePluginId).has_value());
    QCOMPARE(universe.pluginRoot(), QDir(root.path()).absolutePath());

    // Without them, nothing changes.
    QCommandLineParser empty;
    Universe::addCommandLineOptions(empty);
    QVERIFY(empty.parse({QStringLiteral("app")}));
    Universe other;
    const QString pluginRoot = other.pluginRoot();
    other.applyCommandLine(empty);
    QCOMPARE(other.pluginRoot(), pluginRoot);
    QCOMPARE(pluginRoot, QCoreApplication::applicationDirPath());
}

void TestUniverse::unloadPlugin()
{
    Universe universe;
    universe.plugins().scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY(universe.plugins().load(ExamplePluginId));
    QVERIFY(universe.instantiate(GreeterId) != nullptr);
    ::instantiate<Probe>(universe);
    QVERIFY(universe.instantiate(GreeterId) != nullptr);
    QCOMPARE(universe.componentsOf(ExamplePluginId).size(), 2);

    QVERIFY(universe.unloadPlugin(ExamplePluginId));
    QVERIFY(universe.errorString().isEmpty());
    QCOMPARE(names(universe), QStringList{"Probe1"});
    QVERIFY(!universe.plugins().isLoaded(ExamplePluginId));
    QVERIFY(!Nebula::ComponentFactory::instance().component(GreeterId).has_value());

    // Not loaded by it: its components go, but it stays.
    QVERIFY(!universe.unloadPlugin(TestComponents::Id));
    QCOMPARE(universe.errorString(), QStringLiteral("Unknown plugin Test"));
    QVERIFY(universe.components().empty());
}

QTEST_GUILESS_MAIN(TestUniverse)
#include "TestUniverse.moc"
