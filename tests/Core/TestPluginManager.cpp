// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <optional>

#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Plugin/PluginManager.h"

using Nebula::PluginInfo;
using Nebula::PluginManager;

namespace
{

const QString ExamplePluginId = QStringLiteral("FabienDG.Nebula.ExamplePlugin");
const QString ExternalPluginId = QStringLiteral("FabienDG.Nebula.ExternalPlugin");

// The file of ExamplePlugin, as the build wrote it.
QString examplePluginFile()
{
    return QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR "/ExamplePlugin.nbdl");
}

// Whether ComponentFactory::instance() can create componentId.
bool isRegistered(const QString& componentId)
{
    return Nebula::ComponentFactory::instance().component(componentId).has_value();
}

} // namespace

// PluginManager, with the example plugins of the repository: how it finds them, loads and unloads them, and what it
// refuses.
class TestPluginManager : public QObject
{
    Q_OBJECT

  private slots:
    void init();

    void defaultDirectory();
    void scan_data();
    void scan();
    void scanSkipsWhatItKnows();
    void scanReadsOnlyPlugins();
    void addFile_data();
    void addFile();
    void loadAndUnload_data();
    void loadAndUnload();
    void unknownPlugin();
    void unloadWhatIsNotLoaded();
    void pluginLoadedByTwoManagers();
    void pluginLoadedByAnotherManager();
    void destructorUnloads();
};

void TestPluginManager::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestPluginManager::defaultDirectory()
{
    QCOMPARE(PluginManager::defaultDirectory(), QCoreApplication::applicationDirPath() + QStringLiteral("/plugins"));
}

void TestPluginManager::scan_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("id");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("description");

    QTest::newRow("ExamplePlugin") << NEBULA_EXAMPLE_PLUGINS_DIR << ExamplePluginId << "ExamplePlugin"
                                   << "Example plugin: its metadata is read without loading it";
    QTest::newRow("ExternalPlugin") << NEBULA_EXTERNAL_PLUGINS_DIR << ExternalPluginId << "ExternalPlugin"
                                    << "Example plugin outside the plugins directory, opened from NebulaStudio";
}

void TestPluginManager::scan()
{
    QFETCH(QString, directory);
    QFETCH(QString, id);
    QFETCH(QString, name);
    QFETCH(QString, description);

    PluginManager plugins;
    const QList<PluginInfo> found = plugins.scan(directory);
    QCOMPARE(found.size(), 1);
    QCOMPARE(plugins.plugins().size(), 1);

    const std::optional<PluginInfo> info = plugins.plugin(id);
    QVERIFY(info.has_value());
    QCOMPARE(info->id, id);
    QCOMPARE(info->name, name);
    QCOMPARE(info->description, description);
    QCOMPARE(info->version, QVersionNumber(1, 0, 0));
    QCOMPARE(info->coreVersion, QVersionNumber::fromString(QStringLiteral(NEBULA_VERSION)));
    QCOMPARE(info->filePath, QFileInfo(directory + u'/' + name + QStringLiteral(".nbdl")).canonicalFilePath());
    // Read without loading it.
    QVERIFY(!info->buildDate.isValid());
    QVERIFY(!plugins.isLoaded(id));
}

void TestPluginManager::scanSkipsWhatItKnows()
{
    PluginManager plugins;
    QCOMPARE(plugins.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR)).size(), 1);
    QVERIFY(plugins.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR)).isEmpty());

    // The same plugin in another file: skipped, its id being known.
    const QTemporaryDir copies;
    QVERIFY(QFile::copy(examplePluginFile(), copies.filePath(QStringLiteral("Copy.nbdl"))));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^Skipping .*Copy\\.nbdl - %1 is already "
                                                                         "provided by")
                                                              .arg(QRegularExpression::escape(ExamplePluginId))));
    QVERIFY(plugins.scan(copies.path()).isEmpty());
    QCOMPARE(plugins.plugins().size(), 1);
}

void TestPluginManager::scanReadsOnlyPlugins()
{
    const QTemporaryDir directory;
    for(const QString& name : {QStringLiteral("readme.txt"), QStringLiteral("fake.nbdl")})
    {
        QFile file(directory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Not a plugin");
    }

    PluginManager plugins;
    // fake.nbdl only: the others are not even read.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("^Skipping .*fake\\.nbdl - ")));
    QVERIFY(plugins.scan(directory.path()).isEmpty());
    QVERIFY(plugins.scan(directory.filePath(QStringLiteral("missing"))).isEmpty());
    QVERIFY(plugins.plugins().isEmpty());
}

void TestPluginManager::addFile_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("added");
    // A part of errorString(); empty when it is.
    QTest::addColumn<QString>("error");

    QTest::newRow("plugin") << "example" << true << "";
    QTest::newRow("known already") << "example again" << true << "";
    QTest::newRow("missing") << "missing" << false << "is not a file";
    QTest::newRow("directory") << "directory" << false << "is not a file";
    QTest::newRow("not a plugin") << "text" << false << "";
    QTest::newRow("id of another file") << "copy" << false
                                        << ExamplePluginId + QStringLiteral(" is already provided by");
}

void TestPluginManager::addFile()
{
    QFETCH(QString, file);
    QFETCH(bool, added);
    QFETCH(QString, error);

    const QTemporaryDir directory;
    const QString copy = directory.filePath(QStringLiteral("Copy.nbdl"));
    const QString text = directory.filePath(QStringLiteral("text.nbdl"));
    QVERIFY(QFile::copy(examplePluginFile(), copy));
    QFile textFile(text);
    QVERIFY(textFile.open(QIODevice::WriteOnly));
    textFile.write("Not a plugin");
    textFile.close();
    const QHash<QString, QString> paths{
        {QStringLiteral("example"), examplePluginFile()},
        {QStringLiteral("example again"), examplePluginFile()},
        {QStringLiteral("missing"), directory.filePath("none")},
        {QStringLiteral("directory"), directory.path()},
        {QStringLiteral("text"), text},
        {QStringLiteral("copy"), copy},
    };

    PluginManager plugins;
    if(file == u"example again" || file == u"copy")
    {
        QVERIFY(plugins.addFile(examplePluginFile()).has_value());
    }
    const std::optional<PluginInfo> info = plugins.addFile(paths.value(file));
    QCOMPARE(info.has_value(), added);
    if(added)
    {
        QCOMPARE(info->id, ExamplePluginId);
        QVERIFY(plugins.errorString().isEmpty());
    }
    else
    {
        QVERIFY(!plugins.errorString().isEmpty());
        QVERIFY2(plugins.errorString().contains(error), qPrintable(plugins.errorString()));
    }
    QCOMPARE(plugins.plugins().size(), file == u"example again" || file == u"copy" || added ? 1 : 0);
}

void TestPluginManager::loadAndUnload_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("id");
    QTest::addColumn<QStringList>("components");

    QTest::newRow("ExamplePlugin") << NEBULA_EXAMPLE_PLUGINS_DIR << ExamplePluginId << QStringList{"Greeter"};
    QTest::newRow("ExternalPlugin") << NEBULA_EXTERNAL_PLUGINS_DIR << ExternalPluginId
                                    << QStringList{"Ticker", "ProfileSelector", "Window"};
}

void TestPluginManager::loadAndUnload()
{
    QFETCH(QString, directory);
    QFETCH(QString, id);
    QFETCH(QStringList, components);

    PluginManager plugins;
    plugins.scan(directory);
    // Twice: unloaded, it can be loaded again.
    for(int time = 0; time < 2; ++time)
    {
        QVERIFY2(plugins.load(id), qPrintable(plugins.errorString()));
        QVERIFY(plugins.isLoaded(id));
        QVERIFY(plugins.plugin(id)->buildDate.isValid());
        for(const QString& component : components)
        {
            QVERIFY2(isRegistered(id + u'.' + component), qPrintable(component));
        }

        QVERIFY(plugins.unload(id));
        QVERIFY(plugins.errorString().isEmpty());
        QVERIFY(!plugins.isLoaded(id));
        for(const QString& component : components)
        {
            QVERIFY2(!isRegistered(id + u'.' + component), qPrintable(component));
        }
    }
}

void TestPluginManager::unknownPlugin()
{
    PluginManager plugins;
    QVERIFY(!plugins.load(QStringLiteral("Unknown")));
    QCOMPARE(plugins.errorString(), QStringLiteral("Unknown plugin Unknown"));
    QVERIFY(!plugins.unload(QStringLiteral("Unknown")));
    QCOMPARE(plugins.errorString(), QStringLiteral("Unknown plugin Unknown"));
    QVERIFY(!plugins.isLoaded(QStringLiteral("Unknown")));
    QVERIFY(!plugins.plugin(QStringLiteral("Unknown")).has_value());
}

void TestPluginManager::unloadWhatIsNotLoaded()
{
    PluginManager plugins;
    plugins.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY(plugins.unload(ExamplePluginId));
    QVERIFY(plugins.errorString().isEmpty());
}

void TestPluginManager::pluginLoadedByTwoManagers()
{
    const QString greeter = ExamplePluginId + QStringLiteral(".Greeter");
    PluginManager first;
    PluginManager second;
    first.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    second.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY(first.load(ExamplePluginId));
    QVERIFY(second.load(ExamplePluginId));

    // It stays loaded for the second one, its components with it.
    QVERIFY(!first.unload(ExamplePluginId));
    QCOMPARE(first.errorString(), ExamplePluginId + QStringLiteral(" is still loaded by another plugin manager"));
    QVERIFY(first.isLoaded(ExamplePluginId));
    QVERIFY(isRegistered(greeter));

    QVERIFY(second.unload(ExamplePluginId));
    QVERIFY(!second.isLoaded(ExamplePluginId));
    QVERIFY(!first.isLoaded(ExamplePluginId));
    QVERIFY(!isRegistered(greeter));
}

void TestPluginManager::pluginLoadedByAnotherManager()
{
    PluginManager loading;
    PluginManager other;
    loading.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    other.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY(loading.load(ExamplePluginId));

    // Only the one that loaded it unloads it.
    QVERIFY(other.isLoaded(ExamplePluginId));
    QVERIFY(!other.unload(ExamplePluginId));
    QCOMPARE(other.errorString(), ExamplePluginId + QStringLiteral(" was loaded by another plugin manager"));
    QVERIFY(loading.isLoaded(ExamplePluginId));
    QVERIFY(loading.unload(ExamplePluginId));
}

void TestPluginManager::destructorUnloads()
{
    {
        PluginManager plugins;
        plugins.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
        QVERIFY(plugins.load(ExamplePluginId));
        QVERIFY(isRegistered(ExamplePluginId + QStringLiteral(".Greeter")));
    }
    QVERIFY(!isRegistered(ExamplePluginId + QStringLiteral(".Greeter")));

    PluginManager again;
    again.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    QVERIFY(!again.isLoaded(ExamplePluginId));
}

QTEST_GUILESS_MAIN(TestPluginManager)
#include "TestPluginManager.moc"
