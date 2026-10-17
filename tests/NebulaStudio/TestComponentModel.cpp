// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAbstractItemModelTester>
#include <QIcon>
#include <QMimeData>
#include <QSignalSpy>
#include <QTest>
#include <memory>

#include "ComponentModel.h"
#include "Nebula/Component/ComponentFactory.h"
#include "TestComponents.h"

using namespace Qt::StringLiterals;

// ComponentModel, the list of the components of NebulaStudio, over the components of the tests.
class TestComponentModel : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void data_data();
    void data();
    void headerData_data();
    void headerData();
    void dragged();
    void refreshFollowsTheFactory();

  private:
    std::unique_ptr<ComponentModel> m_model;
    std::unique_ptr<QAbstractItemModelTester> m_tester; // Checks every change of m_model
};

void TestComponentModel::initTestCase()
{
    registerTestComponents();
}

void TestComponentModel::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_model = std::make_unique<ComponentModel>(Nebula::ComponentFactory::instance());
    m_tester = std::make_unique<QAbstractItemModelTester>(m_model.get(),
                                                          QAbstractItemModelTester::FailureReportingMode::QtTest);
    m_model->refresh();
}

void TestComponentModel::cleanup()
{
    m_tester.reset();
    m_model.reset();
}

void TestComponentModel::data_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");
    QTest::addColumn<int>("role");
    QTest::addColumn<QVariant>("value");

    // By id: Test.Panel, Test.Probe, Test.Sink, Test.Source.
    QTest::newRow("name") << 1 << int(ComponentModel::Name) << int(Qt::DisplayRole) << QVariant(u"Probe"_s);
    QTest::newRow("category") << 1 << int(ComponentModel::Category) << int(Qt::DisplayRole) << QVariant(u"Test"_s);
    QTest::newRow("no category") << 0 << int(ComponentModel::Category) << int(Qt::DisplayRole) << QVariant(QString());
    QTest::newRow("plugin") << 2 << int(ComponentModel::Plugin) << int(Qt::DisplayRole) << QVariant(u"Test"_s);
    QTest::newRow("description") << 3 << int(ComponentModel::Name) << int(Qt::ToolTipRole)
                                 << QVariant(u"Provides an object while it runs"_s);
    QTest::newRow("id") << 3 << int(ComponentModel::Name) << ComponentModel::IdRole << QVariant(u"Test.Source"_s);
    QTest::newRow("id, in every column") << 0 << int(ComponentModel::Plugin) << ComponentModel::IdRole
                                         << QVariant(u"Test.Panel"_s);
    QTest::newRow("no icon but for the name")
        << 0 << int(ComponentModel::Category) << int(Qt::DecorationRole) << QVariant();
}

void TestComponentModel::data()
{
    QFETCH(int, row);
    QFETCH(int, column);
    QFETCH(int, role);
    QFETCH(QVariant, value);

    QCOMPARE(m_model->rowCount(), 4);
    QCOMPARE(m_model->columnCount(), int(ComponentModel::ColumnCount));
    QCOMPARE(m_model->index(row, column).data(role), value);
    // The icon of an Astre, before each name.
    QVERIFY(!m_model->index(row, ComponentModel::Name).data(Qt::DecorationRole).value<QIcon>().isNull());
}

void TestComponentModel::headerData_data()
{
    QTest::addColumn<int>("section");
    QTest::addColumn<QString>("title");

    QTest::newRow("Name") << int(ComponentModel::Name) << "Name";
    QTest::newRow("Category") << int(ComponentModel::Category) << "Category";
    QTest::newRow("Plugin") << int(ComponentModel::Plugin) << "Plugin";
}

void TestComponentModel::headerData()
{
    QFETCH(int, section);
    QFETCH(QString, title);

    QCOMPARE(m_model->headerData(section, Qt::Horizontal).toString(), title);
    QVERIFY(!m_model->headerData(section, Qt::Vertical).isValid());
}

void TestComponentModel::dragged()
{
    QVERIFY(m_model->flags(m_model->index(1, ComponentModel::Category)).testFlag(Qt::ItemIsDragEnabled));
    QVERIFY(!m_model->flags({}).testFlag(Qt::ItemIsDragEnabled));
    QCOMPARE(m_model->mimeTypes(), QStringList{ComponentModel::MimeType});

    // A row, one index per column: its id, in UTF-8.
    const std::unique_ptr<QMimeData> data(
        m_model->mimeData({m_model->index(1, ComponentModel::Name), m_model->index(1, ComponentModel::Category),
                           m_model->index(1, ComponentModel::Plugin)}));
    QVERIFY(data != nullptr);
    QCOMPARE(data->data(ComponentModel::MimeType), QByteArray("Test.Probe"));
    QCOMPARE(m_model->mimeData({}), nullptr);
}

void TestComponentModel::refreshFollowsTheFactory()
{
    const std::unique_ptr<Nebula::Plugin> other = Nebula::Plugin::instantiate<TestComponents>();
    const QSignalSpy inserted(m_model.get(), &QAbstractItemModel::rowsInserted);
    const QSignalSpy removed(m_model.get(), &QAbstractItemModel::rowsRemoved);

    // Those of a plugin just loaded, where they belong by id: before Test.
    Nebula::ComponentFactory::instance().add(QStringLiteral("Another"), *other);
    m_model->refresh();
    QCOMPARE(m_model->rowCount(), 8);
    QCOMPARE(inserted.count(), 4);
    QCOMPARE(m_model->index(0, ComponentModel::Name).data(ComponentModel::IdRole).toString(), u"Another.Panel"_s);
    QCOMPARE(m_model->index(4, ComponentModel::Name).data(ComponentModel::IdRole).toString(), u"Test.Panel"_s);

    // And no longer those of a plugin just unloaded.
    Nebula::ComponentFactory::instance().remove(QStringLiteral("Another"));
    m_model->refresh();
    QCOMPARE(m_model->rowCount(), 4);
    QCOMPARE(removed.count(), 4);
    QCOMPARE(m_model->index(0, ComponentModel::Name).data(ComponentModel::IdRole).toString(), u"Test.Panel"_s);
}

QTEST_MAIN(TestComponentModel)
#include "TestComponentModel.moc"
