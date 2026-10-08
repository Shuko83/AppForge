#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QVersionNumber>
#include <memory>
#include <optional>

#include "Component/ComponentFactory.h"
#include "Plugin/PluginManager.h"

namespace
{

// TEST_PLUGIN_ID, TEST_PLUGIN_FILE, NEWER_CORE_PLUGIN_FILE and CORE_VERSION are given by tests/CMakeLists.txt.
QString testPluginId()
{
    return QStringLiteral(TEST_PLUGIN_ID);
}

QString testPluginFile()
{
    return QFileInfo(QStringLiteral(TEST_PLUGIN_FILE)).canonicalFilePath();
}

QString newerCorePluginFile()
{
    return QFileInfo(QStringLiteral(NEWER_CORE_PLUGIN_FILE)).canonicalFilePath();
}

// Where appforge_add_plugin wrote the test plugins, NewerCorePlugin included.
QString pluginsDirectory()
{
    return QFileInfo(testPluginFile()).absolutePath();
}

// <name>.<extension appforge_add_plugin gives to the plugins>
QString pluginFileName(const QString& name)
{
    return name + u'.' + QFileInfo(testPluginFile()).suffix();
}

// The error PluginManager gives for NewerCorePlugin, built against the next major version of Core.
QString newerCoreError()
{
    const QString coreVersion = QStringLiteral(CORE_VERSION);
    return QStringLiteral("Built against Core %1.0.0 instead of %2")
        .arg(QString::number(QVersionNumber::fromString(coreVersion).majorVersion() + 1), coreVersion);
}

// Expects the warning scan() gives when it skips NewerCorePlugin.
void ignoreNewerCoreWarning()
{
    const QByteArray message = QStringLiteral("Skipping %1 - %2").arg(newerCorePluginFile(), newerCoreError()).toUtf8();
    QTest::ignoreMessage(QtWarningMsg, message.constData());
}

// Writes a file which is not a plugin, though named like one, in directory, and returns its path.
QString writeNotAPlugin(const QTemporaryDir& directory)
{
    const QString filePath = directory.filePath(pluginFileName(QStringLiteral("NotAPlugin")));
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write("Not a plugin") < 0)
    {
        return {};
    }
    return filePath;
}

} // namespace

class PluginManagerTest : public QObject
{
    Q_OBJECT

  private slots:
    void scanReadsMetadataWithoutLoading();
    void scanReturnsNewPluginsOnly();
    void scanSkipsFilesThatAreNotPlugins();
    void addFileAnywhere();
    void addFileKnownPlugin();
    void addFileRefusesMissingFile();
    void addFileRefusesFileThatIsNotAPlugin();
    void addFileRefusesNewerCore();
    void addFileRefusesKnownId();
    void loadUnknownPlugin();
    // Last: a loaded plugin stays loaded until the process exits.
    void loadRegistersComponents();
};

void PluginManagerTest::scanReadsMetadataWithoutLoading()
{
    AppForge::PluginManager plugins;
    ignoreNewerCoreWarning();
    const QList<AppForge::PluginInfo> found = plugins.scan(pluginsDirectory());
    QCOMPARE(found.size(), 1);

    const AppForge::PluginInfo& info = found.first();
    QCOMPARE(info.id, testPluginId());
    QCOMPARE(info.name, QStringLiteral("TestPlugin"));
    QCOMPARE(info.description, QStringLiteral("Plugin of the \"PluginManager\" tests"));
    QCOMPARE(info.version, QVersionNumber(2, 3, 4));
    QCOMPARE(info.coreVersion, QVersionNumber::fromString(QStringLiteral(CORE_VERSION)));
    QCOMPARE(info.filePath, testPluginFile());
    // Only known once loaded.
    QVERIFY(!info.buildDate.isValid());
    QVERIFY(!plugins.isLoaded(info.id));

    QCOMPARE(plugins.plugins().size(), 1);
    const std::optional<AppForge::PluginInfo> plugin = plugins.plugin(testPluginId());
    QVERIFY(plugin);
    QCOMPARE(plugin->filePath, testPluginFile());
    QVERIFY(!plugins.plugin(QStringLiteral("Tests.Unknown")));
}

void PluginManagerTest::scanReturnsNewPluginsOnly()
{
    AppForge::PluginManager plugins;
    ignoreNewerCoreWarning();
    QCOMPARE(plugins.scan(pluginsDirectory()).size(), 1);

    // NewerCorePlugin was not added: it is skipped again.
    ignoreNewerCoreWarning();
    QVERIFY(plugins.scan(pluginsDirectory()).isEmpty());
    QCOMPARE(plugins.plugins().size(), 1);

    // Added with addFile() first.
    AppForge::PluginManager added;
    QVERIFY(added.addFile(testPluginFile()));
    ignoreNewerCoreWarning();
    QVERIFY(added.scan(pluginsDirectory()).isEmpty());
    QCOMPARE(added.plugins().size(), 1);
}

void PluginManagerTest::scanSkipsFilesThatAreNotPlugins()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString notAPlugin = writeNotAPlugin(directory);
    QVERIFY(!notAPlugin.isEmpty());

    AppForge::PluginManager plugins;
    const QString pattern =
        QStringLiteral("^Skipping %1 - .+").arg(QRegularExpression::escape(QFileInfo(notAPlugin).canonicalFilePath()));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(pattern));
    QVERIFY(plugins.scan(directory.path()).isEmpty());
    QVERIFY(plugins.plugins().isEmpty());

    // An empty or missing directory holds no plugin.
    const QTemporaryDir empty;
    QVERIFY(plugins.scan(empty.path()).isEmpty());
    QVERIFY(plugins.scan(empty.filePath(QStringLiteral("Missing"))).isEmpty());
}

void PluginManagerTest::addFileAnywhere()
{
    AppForge::PluginManager plugins;
    const std::optional<AppForge::PluginInfo> info = plugins.addFile(QStringLiteral(TEST_PLUGIN_FILE));
    QVERIFY2(info, qPrintable(plugins.errorString()));
    QCOMPARE(info->id, testPluginId());
    QCOMPARE(info->filePath, testPluginFile());
    QVERIFY(plugins.errorString().isEmpty());
    QCOMPARE(plugins.plugins().size(), 1);
}

void PluginManagerTest::addFileKnownPlugin()
{
    AppForge::PluginManager plugins;
    ignoreNewerCoreWarning();
    QCOMPARE(plugins.scan(pluginsDirectory()).size(), 1);

    const std::optional<AppForge::PluginInfo> info = plugins.addFile(testPluginFile());
    QVERIFY2(info, qPrintable(plugins.errorString()));
    QCOMPARE(info->id, testPluginId());
    QVERIFY(plugins.errorString().isEmpty());
    QCOMPARE(plugins.plugins().size(), 1);
}

void PluginManagerTest::addFileRefusesMissingFile()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());

    AppForge::PluginManager plugins;
    QVERIFY(!plugins.addFile(directory.filePath(pluginFileName(QStringLiteral("Missing")))));
    QVERIFY2(plugins.errorString().endsWith(QStringLiteral(" is not a file")), qPrintable(plugins.errorString()));
    // Nor is a directory.
    QVERIFY(!plugins.addFile(directory.path()));
    QVERIFY2(plugins.errorString().endsWith(QStringLiteral(" is not a file")), qPrintable(plugins.errorString()));
    QVERIFY(plugins.plugins().isEmpty());
}

void PluginManagerTest::addFileRefusesFileThatIsNotAPlugin()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString notAPlugin = writeNotAPlugin(directory);
    QVERIFY(!notAPlugin.isEmpty());

    AppForge::PluginManager plugins;
    QVERIFY(!plugins.addFile(notAPlugin));
    QVERIFY(!plugins.errorString().isEmpty());
    QVERIFY(plugins.plugins().isEmpty());

    // The error of the last call only.
    QVERIFY(plugins.addFile(testPluginFile()));
    QVERIFY(plugins.errorString().isEmpty());
}

void PluginManagerTest::addFileRefusesNewerCore()
{
    AppForge::PluginManager plugins;
    QVERIFY(!plugins.addFile(newerCorePluginFile()));
    QCOMPARE(plugins.errorString(), newerCoreError());
    QVERIFY(plugins.plugins().isEmpty());
}

void PluginManagerTest::addFileRefusesKnownId()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString copy = directory.filePath(pluginFileName(QStringLiteral("Copy")));
    QVERIFY(QFile::copy(testPluginFile(), copy));

    AppForge::PluginManager plugins;
    QVERIFY(plugins.addFile(testPluginFile()));
    QVERIFY(!plugins.addFile(copy));
    QVERIFY2(plugins.errorString().startsWith(testPluginId() + QStringLiteral(" is already provided by ")),
             qPrintable(plugins.errorString()));
    QCOMPARE(plugins.plugins().size(), 1);
    QCOMPARE(plugins.plugin(testPluginId())->filePath, testPluginFile());
}

void PluginManagerTest::loadUnknownPlugin()
{
    AppForge::PluginManager plugins;
    QVERIFY(!plugins.load(QStringLiteral("Tests.Unknown")));
    QCOMPARE(plugins.errorString(), QStringLiteral("Unknown plugin Tests.Unknown"));
    QVERIFY(!plugins.isLoaded(QStringLiteral("Tests.Unknown")));
}

void PluginManagerTest::loadRegistersComponents()
{
    const QString probeId = testPluginId() + QStringLiteral(".Probe");
    AppForge::ComponentFactory& factory = AppForge::ComponentFactory::instance();
    QVERIFY(!factory.component(probeId));

    AppForge::PluginManager plugins;
    QVERIFY(plugins.addFile(testPluginFile()));
    QVERIFY(!plugins.isLoaded(testPluginId()));
    QVERIFY2(plugins.load(testPluginId()), qPrintable(plugins.errorString()));
    QVERIFY(plugins.isLoaded(testPluginId()));
    QVERIFY(plugins.errorString().isEmpty());

    const QDateTime buildDate = plugins.plugin(testPluginId())->buildDate;
    QVERIFY(buildDate.isValid());
    QVERIFY(buildDate <= QDateTime::currentDateTime());

    const std::optional<AppForge::ComponentInfo> probe = factory.component(probeId);
    QVERIFY(probe);
    QCOMPARE(probe->pluginId, testPluginId());
    QCOMPARE(probe->name, QStringLiteral("Probe"));
    QCOMPARE(probe->description, QStringLiteral("Component of the PluginManager tests"));
    QCOMPARE(probe->category, QStringLiteral("Tests"));

    // Built by the class of the plugin, through its root object.
    const std::unique_ptr<AppForge::Component> component = factory.create(probeId);
    QVERIFY(component);
    QCOMPARE(component->componentId(), probeId);
    QCOMPARE(component->property("builtBy").toString(), QStringLiteral("TestPlugin"));

    // Loading it again registers nothing more.
    const qsizetype count = factory.components().size();
    QVERIFY(plugins.load(testPluginId()));
    QCOMPARE(factory.components().size(), count);
}

QTEST_GUILESS_MAIN(PluginManagerTest)

#include "PluginManagerTest.moc"
