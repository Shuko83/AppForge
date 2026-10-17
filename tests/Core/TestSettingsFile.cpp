// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <memory>
#include <utility>

#include "Nebula/Application/Universe.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Options.h"
#include "Panel.h"
#include "Probe.h"
#include "TestComponents.h"

using Nebula::SettingsEntry;
using Nebula::SettingsFile;
using Nebula::Universe;

namespace
{

// The JSON object the file filePath holds; empty when it holds none.
QJsonObject readJson(const QString& filePath)
{
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

// Writes content to the file filePath.
bool writeFile(const QString& filePath, const QByteArray& content)
{
    QFile file(filePath);
    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

// The JSON object text holds.
QJsonObject json(const QByteArray& text)
{
    return QJsonDocument::fromJson(text).object();
}

} // namespace

// SettingsFile: its name and its path, the properties it can hold, and what it reads and writes while the application
// runs.
class TestSettingsFile : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void filePath_data();
    void filePath();
    void setFileName_data();
    void setFileName();
    void setDirectory();
    void canHold_data();
    void canHold();
    void add_data();
    void add();
    void remove();
    void entryLeavesWithItsObject();

    void startWritesWhatIsMissing();
    void startReadsTheValues_data();
    void startReadsTheValues();
    void startRefusesAValueOfAnotherType();
    void startSetsAsideAnUnreadableFile_data();
    void startSetsAsideAnUnreadableFile();
    void unknownContentIsKept();
    void changesAreWrittenWhileRunning();
    void writeDefaults();

  private:
    // The application of each test, with a Probe1 and a Panel1, and a settings file in m_directory.
    std::unique_ptr<Universe> m_universe;
    Probe* m_probe = nullptr;
    Panel* m_panel = nullptr;
    SettingsFile* m_file = nullptr;
    std::unique_ptr<QTemporaryDir> m_directory;
};

void TestSettingsFile::initTestCase()
{
    registerTestComponents();
    // A file without directory is written there.
    QStandardPaths::setTestModeEnabled(true);
}

void TestSettingsFile::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_directory = std::make_unique<QTemporaryDir>();
    QVERIFY(m_directory->isValid());
    m_universe = std::make_unique<Universe>();
    m_probe = qobject_cast<Probe*>(m_universe->instantiate(testComponentId(QStringLiteral("Probe"))));
    m_panel = qobject_cast<Panel*>(m_universe->instantiate(testComponentId(QStringLiteral("Panel"))));
    QVERIFY(m_probe != nullptr && m_panel != nullptr);
    m_file = &m_universe->addSettingsFile();
    m_file->setDirectory(m_directory->path());
}

void TestSettingsFile::cleanup()
{
    m_universe.reset();
    m_directory.reset();
}

void TestSettingsFile::filePath_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("filePath");

    const QString application = QCoreApplication::applicationDirPath();
    QTest::newRow("no directory") << ""
                                  << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
                                         QStringLiteral("/s.json");
    QTest::newRow("relative") << "config" << application + QStringLiteral("/config/s.json");
    QTest::newRow("relative, above") << "../config"
                                     << QDir::cleanPath(application + QStringLiteral("/../config/s.json"));
    QTest::newRow("absolute") << QDir::tempPath() + QStringLiteral("/config")
                              << QDir::tempPath() + QStringLiteral("/config/s.json");
}

void TestSettingsFile::filePath()
{
    QFETCH(QString, directory);
    QFETCH(QString, filePath);

    QCOMPARE(SettingsFile::filePath(QStringLiteral("s.json"), directory), filePath);
    QVERIFY(m_file->setFileName(QStringLiteral("s.json")));
    m_file->setDirectory(directory);
    QCOMPARE(m_file->filePath(), filePath);
}

void TestSettingsFile::setFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("set");
    QTest::addColumn<bool>("changed");

    QTest::newRow("new name") << "new.json" << true << true;
    QTest::newRow("its own name") << "settings.json" << true << false;
    QTest::newRow("its own name, in another case") << "SETTINGS.json" << true << true;
    QTest::newRow("name of another") << "settings2.json" << false << false;
    // Two names that only differ by their case are the same file where file names do not tell it apart.
    QTest::newRow("name of another, in another case") << "Settings2.JSON" << false << false;
    QTest::newRow("empty") << "" << false << false;
}

void TestSettingsFile::setFileName()
{
    QFETCH(QString, fileName);
    QFETCH(bool, set);
    QFETCH(bool, changed);

    QCOMPARE(m_universe->addSettingsFile().fileName(), QStringLiteral("settings2.json"));
    const QSignalSpy changedSpy(m_file, &SettingsFile::changed);
    QCOMPARE(m_file->setFileName(fileName), set);
    QCOMPARE(m_file->fileName(), set ? fileName : QStringLiteral("settings.json"));
    QCOMPARE(changedSpy.count(), changed ? 1 : 0);
}

void TestSettingsFile::setDirectory()
{
    const QSignalSpy changed(m_file, &SettingsFile::changed);
    m_file->setDirectory(QStringLiteral("config"));
    m_file->setDirectory(QStringLiteral("config"));
    QCOMPARE(m_file->directory(), QStringLiteral("config"));
    QCOMPARE(changed.count(), 1);
}

void TestSettingsFile::canHold_data()
{
    QTest::addColumn<QString>("object");
    QTest::addColumn<QString>("property");
    QTest::addColumn<bool>("held");

    QTest::newRow("int") << "probe" << "count" << true;
    QTest::newRow("QDateTime") << "probe" << "moment" << true;
    QTest::newRow("JSON has nothing for it") << "probe" << "point" << false;
    QTest::newRow("read only") << "probe" << "starts" << false;
    QTest::newRow("of Component") << "probe" << "componentId" << false;
    QTest::newRow("name of the instance") << "probe" << "objectName" << false;
    QTest::newRow("unknown") << "probe" << "unknown" << false;
    QTest::newRow("of an object") << "general" << "level" << true;
    QTest::newRow("of an object, DESIGNABLE false") << "general" << "hint" << false;
    QTest::newRow("of an object, STORED false") << "general" << "cache" << false;
    QTest::newRow("of an object, its name") << "general" << "objectName" << false;
    QTest::newRow("of another object") << "other" << "level" << false;
}

void TestSettingsFile::canHold()
{
    QFETCH(QString, object);
    QFETCH(QString, property);
    QFETCH(bool, held);

    Options other;
    const QHash<QString, std::pair<const Nebula::Component*, const QObject*>> objects{
        {QStringLiteral("probe"), {m_probe, m_probe}},
        {QStringLiteral("general"), {m_panel, &m_panel->general()}},
        {QStringLiteral("other"), {m_panel, &other}},
    };
    const auto [component, holder] = objects.value(object);
    QCOMPARE(SettingsFile::canHold(*component, holder, property), held);
}

void TestSettingsFile::add_data()
{
    QTest::addColumn<QString>("entry");
    QTest::addColumn<bool>("added");

    QTest::newRow("property of a component") << "count" << true;
    QTest::newRow("property of an object") << "page level" << true;
    QTest::newRow("property it cannot hold") << "point" << false;
    QTest::newRow("component of another application") << "foreign" << false;
    QTest::newRow("no component") << "none" << false;
    QTest::newRow("held already") << "held" << false;
    QTest::newRow("held by another file") << "held elsewhere" << false;
}

void TestSettingsFile::add()
{
    QFETCH(QString, entry);
    QFETCH(bool, added);

    Universe foreignUniverse;
    Nebula::Component* foreign = foreignUniverse.instantiate(testComponentId(QStringLiteral("Probe")));
    const SettingsEntry count{.component = m_probe, .object = m_probe, .property = QStringLiteral("count")};
    const SettingsEntry text{.component = m_probe, .object = m_probe, .property = QStringLiteral("text")};
    QVERIFY(m_file->add(count));
    QVERIFY(m_universe->addSettingsFile().add(text));
    const QHash<QString, SettingsEntry> entries{
        {QStringLiteral("count"), {.component = m_probe, .object = m_probe, .property = QStringLiteral("ratio")}},
        {QStringLiteral("page level"),
         {.component = m_panel, .object = m_panel->pages().at(1), .property = QStringLiteral("level")}},
        {QStringLiteral("point"), {.component = m_probe, .object = m_probe, .property = QStringLiteral("point")}},
        {QStringLiteral("foreign"), {.component = foreign, .object = foreign, .property = QStringLiteral("count")}},
        {QStringLiteral("none"), {.component = nullptr, .object = m_probe, .property = QStringLiteral("count")}},
        {QStringLiteral("held"), count},
        {QStringLiteral("held elsewhere"), text},
    };
    const QSignalSpy changed(m_file, &SettingsFile::changed);

    QCOMPARE(m_file->add(entries.value(entry)), added);
    QCOMPARE(changed.count(), added ? 1 : 0);
    QCOMPARE(m_file->entries().size(), added ? 2 : 1);
    QCOMPARE(m_file->contains(entries.value(entry)), added || entry == u"held");
    if(added)
    {
        QCOMPARE(m_universe->settingsFile(entries.value(entry)), m_file);
    }
}

void TestSettingsFile::remove()
{
    const SettingsEntry count{.component = m_probe, .object = m_probe, .property = QStringLiteral("count")};
    QVERIFY(m_file->add(count));
    const QSignalSpy changed(m_file, &SettingsFile::changed);

    m_file->remove(count);
    m_file->remove(count); // Not held any longer: nothing changes
    QVERIFY(m_file->entries().isEmpty());
    QCOMPARE(changed.count(), 1);
    QCOMPARE(m_universe->settingsFile(count), nullptr);
}

void TestSettingsFile::entryLeavesWithItsObject()
{
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(m_file->add({.component = m_panel, .object = m_panel->pages().at(0), .property = QStringLiteral("level")}));
    QVERIFY(m_file->add({.component = m_panel, .object = m_panel->pages().at(1), .property = QStringLiteral("level")}));
    const QSignalSpy changed(m_file, &SettingsFile::changed);

    delete m_panel->pages().at(0);
    QCOMPARE(m_file->entries().size(), 2);
    QVERIFY(m_universe->remove(*m_panel));
    QCOMPARE(m_file->entries().size(), 1);
    QCOMPARE(m_file->entries().first().object, m_probe);
    QCOMPARE(changed.count(), 2);
}

void TestSettingsFile::startWritesWhatIsMissing()
{
    m_probe->setProperty("count", 5);
    m_panel->pages().at(1)->setProperty("level", 4);
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(m_file->add({.component = m_panel, .object = m_panel->pages().at(1), .property = QStringLiteral("level")}));
    QVERIFY(!QFile::exists(m_file->filePath()));

    QVERIFY(m_universe->start());
    // As a project holds them: the second object of its name after a first one.
    QCOMPARE(readJson(m_file->filePath()), json(R"({
        "Probe1": {"properties": {"count": 5}},
        "Panel1": {"objects": [{"name": "page", "properties": {}}, {"name": "page", "properties": {"level": 4}}]}})"));
    m_universe->stop();
}

void TestSettingsFile::startReadsTheValues_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QByteArray>("value"); // As the file holds it
    QTest::addColumn<QVariant>("read");

    QTest::newRow("int") << "count" << QByteArray("7") << QVariant(7);
    QTest::newRow("double") << "ratio" << QByteArray("0.5") << QVariant(0.5);
    QTest::newRow("QString") << "text" << QByteArray(R"("from the file")") << QVariant(QStringLiteral("from the file"));
    QTest::newRow("QDate") << "date" << QByteArray(R"("2026-10-17")") << QVariant(QDate(2026, 10, 17));
    QTest::newRow("QTime") << "time" << QByteArray(R"("12:34:56")") << QVariant(QTime(12, 34, 56));
}

void TestSettingsFile::startReadsTheValues()
{
    QFETCH(QString, property);
    QFETCH(QByteArray, value);
    QFETCH(QVariant, read);

    const QVariant edited = m_probe->property(qPrintable(property));
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = property}));
    QVERIFY(writeFile(m_file->filePath(),
                      R"({"Probe1": {"properties": {")" + property.toUtf8() + R"(": )" + value + "}}}"));
    const QSignalSpy failed(m_file, &SettingsFile::failed);

    QVERIFY(m_universe->start());
    QCOMPARE(m_probe->property(qPrintable(property)), read);
    // The value of the project again.
    m_universe->stop();
    QCOMPARE(m_probe->property(qPrintable(property)), edited);
    QCOMPARE(failed.count(), 0);
}

void TestSettingsFile::startRefusesAValueOfAnotherType()
{
    m_probe->setProperty("count", 3);
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(writeFile(m_file->filePath(), R"({"Probe1": {"properties": {"count": {"not": "an int"}}}})"));
    const QSignalSpy failed(m_universe.get(), &Universe::settingsFileFailed);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("holds for count of Probe1 a value it cannot "
                                                                         "have: it is given its default value$")));
    QVERIFY(m_universe->start());
    QCOMPARE(failed.count(), 1);
    QCOMPARE(m_probe->property("count").toInt(), 3);
    // Written again with its default value.
    QCOMPARE(readJson(m_file->filePath()), json(R"({"Probe1": {"properties": {"count": 3}}})"));
    m_universe->stop();
}

void TestSettingsFile::startSetsAsideAnUnreadableFile_data()
{
    QTest::addColumn<QByteArray>("content");

    QTest::newRow("not JSON") << QByteArray("{ count");
    QTest::newRow("not an object") << QByteArray("[1, 2]");
}

void TestSettingsFile::startSetsAsideAnUnreadableFile()
{
    QFETCH(QByteArray, content);

    m_probe->setProperty("count", 3);
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(writeFile(m_file->filePath(), content));
    const QSignalSpy failed(m_file, &SettingsFile::failed);

    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("cannot be read .*: it is set aside as .*\\.bak "
                                                           "and written again$")));
    QVERIFY(m_universe->start());
    QCOMPARE(failed.count(), 1);
    QFile backup(m_file->filePath() + QStringLiteral(".bak"));
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(backup.readAll(), content);
    QCOMPARE(readJson(m_file->filePath()), json(R"({"Probe1": {"properties": {"count": 3}}})"));
    m_universe->stop();
}

void TestSettingsFile::unknownContentIsKept()
{
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    // Another application can share the file: what it holds of other instances and properties stays.
    QVERIFY(
        writeFile(m_file->filePath(),
                  R"({"Other": {"properties": {"x": 1}}, "Probe1": {"properties": {"text": "kept", "count": 2}}})"));

    QVERIFY(m_universe->start());
    m_probe->setProperty("count", 8);
    m_universe->stop();
    QCOMPARE(readJson(m_file->filePath()),
             json(R"({"Other": {"properties": {"x": 1}}, "Probe1": {"properties": {"text": "kept", "count": 8}}})"));
}

void TestSettingsFile::changesAreWrittenWhileRunning()
{
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(m_file->add({.component = m_panel, .object = &m_panel->general(), .property = QStringLiteral("level")}));
    QVERIFY(m_universe->start());

    m_probe->setProperty("count", 9);
    m_panel->general().setProperty("level", 2);
    // Once the events are processed: the changes made meanwhile are written together.
    QTRY_COMPARE(readJson(m_file->filePath()), json(R"({"Probe1": {"properties": {"count": 9}},
        "Panel1": {"objects": [{"name": "", "properties": {"level": 2}}]}})"));

    m_universe->stop();
    QCOMPARE(m_probe->property("count").toInt(), 0);
    QCOMPARE(m_panel->general().property("level").toInt(), 0);
    // The values of the user, the next time it runs.
    QVERIFY(m_universe->start());
    QCOMPARE(m_probe->property("count").toInt(), 9);
    QCOMPARE(m_panel->general().property("level").toInt(), 2);
    m_universe->stop();
}

void TestSettingsFile::writeDefaults()
{
    m_probe->setProperty("count", 4);
    QVERIFY(m_file->add({.component = m_probe, .object = m_probe, .property = QStringLiteral("count")}));
    QVERIFY(writeFile(m_file->filePath(), R"({"Other": 1, "Probe1": {"properties": {"count": 9}}})"));

    QVERIFY(m_file->writeDefaults());
    QCOMPARE(readJson(m_file->filePath()), json(R"({"Other": 1, "Probe1": {"properties": {"count": 4}}})"));

    QVERIFY(m_universe->start());
    QVERIFY(!m_file->writeDefaults());
    QCOMPARE(m_file->errorString(), QStringLiteral("The default values cannot be written while the application runs"));
    m_universe->stop();
}

QTEST_GUILESS_MAIN(TestSettingsFile)
#include "TestSettingsFile.moc"
