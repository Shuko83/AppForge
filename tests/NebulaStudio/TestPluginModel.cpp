// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>
#include <memory>

#include "Nebula/Plugin/PluginManager.h"
#include "PluginModel.h"

using namespace Qt::StringLiterals;

namespace
{

const QString ExamplePluginId = QStringLiteral("FabienDG.Nebula.ExamplePlugin");
const QString ExternalPluginId = QStringLiteral("FabienDG.Nebula.ExternalPlugin");

} // namespace

// PluginModel, the list of the plugins of NebulaStudio, over the example plugins of the repository.
class TestPluginModel : public QObject
{
    Q_OBJECT

  private slots:
    void init();
    void cleanup();

    void data_data();
    void data();
    void headerData_data();
    void headerData();
    void refreshInsertsNewPlugins();
    void refreshUpdatesTheState();
    void indexOf();

  private:
    std::unique_ptr<Nebula::PluginManager> m_plugins;
    std::unique_ptr<PluginModel> m_model;
    std::unique_ptr<QAbstractItemModelTester> m_tester; // Checks every change of m_model
};

void TestPluginModel::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_plugins = std::make_unique<Nebula::PluginManager>();
    m_model = std::make_unique<PluginModel>(*m_plugins);
    m_tester = std::make_unique<QAbstractItemModelTester>(m_model.get(),
                                                          QAbstractItemModelTester::FailureReportingMode::QtTest);
    m_plugins->scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    m_plugins->scan(QStringLiteral(NEBULA_EXTERNAL_PLUGINS_DIR));
    m_model->refresh();
}

void TestPluginModel::cleanup()
{
    m_tester.reset();
    m_model.reset();
    m_plugins.reset();
}

void TestPluginModel::data_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("role");
    QTest::addColumn<QVariant>("value");

    // By id: ExamplePlugin, then ExternalPlugin.
    QTest::newRow("name") << 0 << int(PluginModel::Name) << int(Qt::DisplayRole) << QVariant(u"ExamplePlugin"_s);
    QTest::newRow("name of the second") << 1 << int(PluginModel::Name) << int(Qt::DisplayRole)
                                        << QVariant(u"ExternalPlugin"_s);
    QTest::newRow("version") << 0 << int(PluginModel::Version) << int(Qt::DisplayRole) << QVariant(u"1.0.0"_s);
    QTest::newRow("state") << 0 << int(PluginModel::State) << int(Qt::DisplayRole) << QVariant(u"Not loaded"_s);
    QTest::newRow("id") << 0 << int(PluginModel::Name) << PluginModel::IdRole << QVariant(ExamplePluginId);
    QTest::newRow("id, in every column") << 1 << int(PluginModel::State) << PluginModel::IdRole
                                         << QVariant(ExternalPluginId);
    QTest::newRow("tooltip") << 0 << int(PluginModel::Version) << int(Qt::ToolTipRole) << QVariant(ExamplePluginId);
    QTest::newRow("other role") << 0 << int(PluginModel::Name) << int(Qt::DecorationRole) << QVariant();
}

void TestPluginModel::data()
{
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(int, role);
    QFETCH(QVariant, value);

    QCOMPARE(m_model->rowCount(), 2);
    QCOMPARE(m_model->columnCount(), int(PluginModel::ColumnCount));
    QCOMPARE(m_model->index(row, column).data(role), value);
}

void TestPluginModel::headerData_data()
{
    QTest::addColumn<int>("section");
    QTest::addColumn<QString>("title");

    QTest::newRow("Name") << int(PluginModel::Name) << "Name";
    QTest::newRow("Version") << int(PluginModel::Version) << "Version";
    QTest::newRow("State") << int(PluginModel::State) << "State";
}

void TestPluginModel::headerData()
{
    QFETCH(int, section);
    QFETCH(QString, title);

    QCOMPARE(m_model->headerData(section, Qt::Horizontal).toString(), title);
    QVERIFY(!m_model->headerData(section, Qt::Vertical).isValid());
}

void TestPluginModel::refreshInsertsNewPlugins()
{
    Nebula::PluginManager plugins;
    PluginModel model(plugins);
    const QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    const QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QCOMPARE(model.rowCount(), 0);

    // The second one by id first: the first one is inserted before it.
    plugins.scan(QStringLiteral(NEBULA_EXTERNAL_PLUGINS_DIR));
    model.refresh();
    plugins.scan(QStringLiteral(NEBULA_EXAMPLE_PLUGINS_DIR));
    model.refresh();
    QCOMPARE(inserted.count(), 2);
    QCOMPARE(inserted.last().at(1).toInt(), 0);
    QCOMPARE(model.index(0, PluginModel::Name).data().toString(), QStringLiteral("ExamplePlugin"));
    QCOMPARE(model.index(1, PluginModel::Name).data().toString(), QStringLiteral("ExternalPlugin"));
}

void TestPluginModel::refreshUpdatesTheState()
{
    const QSignalSpy dataChanged(m_model.get(), &QAbstractItemModel::dataChanged);
    QVERIFY(m_plugins->load(ExamplePluginId));
    m_model->refresh();
    QCOMPARE(dataChanged.count(), 1);
    QCOMPARE(m_model->index(0, PluginModel::State).data().toString(), QStringLiteral("Loaded"));
    QCOMPARE(m_model->index(1, PluginModel::State).data().toString(), QStringLiteral("Not loaded"));
}

void TestPluginModel::indexOf()
{
    QCOMPARE(m_model->indexOf(ExternalPluginId), m_model->index(1, 0));
    QVERIFY(!m_model->indexOf(QStringLiteral("Unknown")).isValid());
}

// A QGuiApplication: QAbstractItemModelTester reads the roles of fonts and pixmaps too.
QTEST_MAIN(TestPluginModel)
#include "TestPluginModel.moc"
