#pragma once

#include <QtTest/QtTest>
#include <QObject>

#include "AfCore/Version.h"

#include <compare>

using namespace appforge::core;

class TestVersion : public QObject
{
    Q_OBJECT
  public:
  void testVersion_data();
  void testVersion();
    {
        Version v1(1, 2, 3);
        Version v2(1, 2, 3);
        Version v3(1, 2, 4);
        Version v4(1, 3, 0);
        Version v5(2, 0, 0);
        QCOMPARE(v1.toString(), QString("1.2.3"));
        QCOMPARE(v1, v2);
        QVERIFY(v1 < v3);
        QVERIFY(v3 < v4);
        QVERIFY(v4 < v5);
    }
  private:
    
};

void TestVersion::testVersion_data()
{
  QTest::addColumn<Version>("v1");
  QTest::addColumn<Version>("v2");
  QTest::addColumn<std::strong_ordering>("order");

  Qtest::newRow("Major diff") << Version{1,2,3} << Version{2,0,0} << std::strong_ordering::less;
  Qtest::newRow("Minor diff") << Version{1,2,3} << Version{1,3,4} << std::strong_ordering::less;
  Qtest::newRow("Patch diff") << Version{1,2,3} << Version{1,2,4} << std::strong_ordering::less;
  Qtest::newRow("Same version") << Version{1,2,3} << Version{1,2,3} << std::strong_ordering::equal;
  
}

void TestVersion::testVersion()
{
  QFETCH(Version, v1);
  QFETCH(Version, v2);
  QFETCH(std::strong_ordering, order);
 
  QCOMPARE( v1 <=> v2, order);
}

QTEST_MAIN(TestVersion)