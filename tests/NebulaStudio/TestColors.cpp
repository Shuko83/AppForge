// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QTest>

#include "Colors.h"

namespace
{

// The precision of the values WCAG gives.
constexpr qreal Precision = 0.005;

} // namespace

// The color functions of NebulaStudio, Colors.h: what WCAG defines, and the color of a text over a background.
class TestColors : public QObject
{
    Q_OBJECT

  private slots:
    void init();

    void over_data();
    void over();
    void luminance_data();
    void luminance();
    void contrast_data();
    void contrast();
    void textColorOver_data();
    void textColorOver();
    void textColorOverIsReadable_data();
    void textColorOverIsReadable();
};

void TestColors::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestColors::over_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QColor>("background");
    QTest::addColumn<QColor>("seen");

    QTest::newRow("opaque") << QColor(0x6D28D9) << QColor(Qt::white) << QColor(0x6D28D9);
    QTest::newRow("transparent") << QColor(0, 0, 0, 0) << QColor(0x15122B) << QColor(0x15122B);
    QTest::newRow("half white over black") << QColor(255, 255, 255, 128) << QColor(Qt::black) << QColor(128, 128, 128);
    QTest::newRow("quarter red over white") << QColor(255, 0, 0, 64) << QColor(Qt::white) << QColor(255, 191, 191);
}

void TestColors::over()
{
    QFETCH(QColor, color);
    QFETCH(QColor, background);
    QFETCH(QColor, seen);

    const QColor result = ::over(color, background);
    QCOMPARE(result.alpha(), 255);
    // Within one step of each channel: the alpha of a QColor is stored on 16 bits.
    QVERIFY2(qAbs(result.red() - seen.red()) <= 1, qPrintable(result.name()));
    QVERIFY2(qAbs(result.green() - seen.green()) <= 1, qPrintable(result.name()));
    QVERIFY2(qAbs(result.blue() - seen.blue()) <= 1, qPrintable(result.name()));
}

void TestColors::luminance_data()
{
    QTest::addColumn<QColor>("color");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("black") << QColor(Qt::black) << 0.0;
    QTest::newRow("white") << QColor(Qt::white) << 1.0;
    QTest::newRow("red") << QColor(255, 0, 0) << 0.2126;
    QTest::newRow("green") << QColor(0, 255, 0) << 0.7152;
    QTest::newRow("blue") << QColor(0, 0, 255) << 0.0722;
    QTest::newRow("middle gray") << QColor(0x808080) << 0.2159;
    // Below 0.04045, a channel is linear.
    QTest::newRow("very dark gray") << QColor(0x0A0A0A) << 10 / 255.0 / 12.92;
}

void TestColors::luminance()
{
    QFETCH(QColor, color);
    QFETCH(qreal, expected);

    QVERIFY2(qAbs(::luminance(color) - expected) < Precision, qPrintable(QString::number(::luminance(color))));
}

void TestColors::contrast_data()
{
    QTest::addColumn<QColor>("first");
    QTest::addColumn<QColor>("second");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("black and white") << QColor(Qt::black) << QColor(Qt::white) << 21.0;
    QTest::newRow("white and black") << QColor(Qt::white) << QColor(Qt::black) << 21.0;
    QTest::newRow("the same color") << QColor(0x6D28D9) << QColor(0x6D28D9) << 1.0;
    // The lightest gray on white that meets AA for a text.
    QTest::newRow("#767676 on white") << QColor(0x767676) << QColor(Qt::white) << 4.54;
    QTest::newRow("#777777 on white") << QColor(0x777777) << QColor(Qt::white) << 4.48;
    QTest::newRow("blue on white") << QColor(0, 0, 255) << QColor(Qt::white) << 8.59;
}

void TestColors::contrast()
{
    QFETCH(QColor, first);
    QFETCH(QColor, second);
    QFETCH(qreal, expected);

    QVERIFY2(qAbs(::contrast(first, second) - expected) < Precision,
             qPrintable(QString::number(::contrast(first, second))));
}

void TestColors::textColorOver_data()
{
    QTest::addColumn<QColor>("background");
    QTest::addColumn<QColor>("text");

    QTest::newRow("white") << QColor(Qt::white) << QColor(Qt::black);
    QTest::newRow("black") << QColor(Qt::black) << QColor(Qt::white);
    QTest::newRow("dark violet") << QColor(0x6D28D9) << QColor(Qt::white);
    QTest::newRow("light violet") << QColor(0xA78BFA) << QColor(Qt::black);
    QTest::newRow("amber") << QColor(0xFBBF24) << QColor(Qt::black);
    QTest::newRow("blue") << QColor(0x3B82F6) << QColor(Qt::black);
}

void TestColors::textColorOver()
{
    QFETCH(QColor, background);
    QFETCH(QColor, text);

    QCOMPARE(::textColorOver(background), text);
}

void TestColors::textColorOverIsReadable_data()
{
    QTest::addColumn<int>("red");

    for(int red = 0; red <= 255; red += 15)
    {
        QTest::addRow("red %d", red) << red;
    }
}

void TestColors::textColorOverIsReadable()
{
    QFETCH(int, red);

    // What it promises, over any background: 4.58 at least, more than the 4.5 WCAG AA asks for a text.
    constexpr qreal Least = 4.58;
    for(int green = 0; green <= 255; green += 15)
    {
        for(int blue = 0; blue <= 255; blue += 15)
        {
            const QColor background(red, green, blue);
            QVERIFY2(::contrast(::textColorOver(background), background) >= Least - Precision,
                     qPrintable(background.name()));
        }
    }
}

QTEST_APPLESS_MAIN(TestColors)
#include "TestColors.moc"
