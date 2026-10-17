// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyle>
#include <QTest>

#include "Colors.h"
#include "Theme.h"

namespace
{

// The token of ThemeColors named name.
QColor token(const ThemeColors& colors, const QString& name)
{
    const QHash<QString, QColor ThemeColors::*> tokens{
        {QStringLiteral("space"), &ThemeColors::space},
        {QStringLiteral("field"), &ThemeColors::field},
        {QStringLiteral("alternateField"), &ThemeColors::alternateField},
        {QStringLiteral("surface"), &ThemeColors::surface},
        {QStringLiteral("edge"), &ThemeColors::edge},
        {QStringLiteral("text"), &ThemeColors::text},
        {QStringLiteral("mutedText"), &ThemeColors::mutedText},
        {QStringLiteral("disabledText"), &ThemeColors::disabledText},
        {QStringLiteral("accent"), &ThemeColors::accent},
        {QStringLiteral("onAccent"), &ThemeColors::onAccent},
        {QStringLiteral("link"), &ThemeColors::link},
        {QStringLiteral("visitedLink"), &ThemeColors::visitedLink},
        {QStringLiteral("warning"), &ThemeColors::warning},
        {QStringLiteral("onWarning"), &ThemeColors::onWarning},
    };
    return colors.*tokens.value(name);
}

} // namespace

// The theme of NebulaStudio, Theme.h: its colors meet WCAG AA, and its resources are there.
class TestTheme : public QObject
{
    Q_OBJECT

  private slots:
    void init();

    void contrast_data();
    void contrast();
    void unknownSchemeIsDark();
    void applyTheme();
    void titleFont();
    void icon_data();
    void icon();
    void logo();
};

void TestTheme::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
}

void TestTheme::contrast_data()
{
    QTest::addColumn<Qt::ColorScheme>("scheme");
    QTest::addColumn<QString>("foreground");
    QTest::addColumn<QString>("background");
    QTest::addColumn<qreal>("least");

    // WCAG AA: 4.5 for a text, 3 for the edges of what can be used.
    constexpr qreal Text = 4.5;
    constexpr qreal Edge = 3;
    const QStringList backgrounds{"space", "field", "alternateField", "surface"};
    for(const auto& [scheme, schemeName] :
        {std::pair{Qt::ColorScheme::Dark, "dark"}, std::pair{Qt::ColorScheme::Light, "light"}})
    {
        const auto add = [&](const QString& foreground, const QString& background, qreal least)
        {
            QTest::addRow("%s: %s on %s", schemeName, qPrintable(foreground), qPrintable(background))
                << scheme << foreground << background << least;
        };
        for(const QString& background : backgrounds)
        {
            add(QStringLiteral("text"), background, Text);
            add(QStringLiteral("mutedText"), background, Text);
            add(QStringLiteral("edge"), background, Edge);
        }
        add(QStringLiteral("onAccent"), QStringLiteral("accent"), Text);
        add(QStringLiteral("onWarning"), QStringLiteral("warning"), Text);
        // A warning is written on the surface and the fields as a text is.
        add(QStringLiteral("warning"), QStringLiteral("surface"), Text);
        add(QStringLiteral("warning"), QStringLiteral("field"), Text);
        add(QStringLiteral("link"), QStringLiteral("field"), Text);
        add(QStringLiteral("visitedLink"), QStringLiteral("field"), Text);
        // The selection and the focus stand out from the fields.
        add(QStringLiteral("accent"), QStringLiteral("field"), Edge);
    }
}

void TestTheme::contrast()
{
    QFETCH(Qt::ColorScheme, scheme);
    QFETCH(QString, foreground);
    QFETCH(QString, background);
    QFETCH(qreal, least);

    const ThemeColors& colors = themeColors(scheme);
    const qreal ratio = ::contrast(token(colors, foreground), token(colors, background));
    QVERIFY2(ratio >= least, qPrintable(QStringLiteral("%1 to 1").arg(ratio, 0, 'f', 2)));
}

void TestTheme::unknownSchemeIsDark()
{
    QCOMPARE(&themeColors(Qt::ColorScheme::Unknown), &themeColors(Qt::ColorScheme::Dark));
    QVERIFY(themeColors(Qt::ColorScheme::Light).space != themeColors(Qt::ColorScheme::Dark).space);
}

void TestTheme::applyTheme()
{
    ::applyTheme();
    QCOMPARE(QApplication::style()->name().toLower(), QStringLiteral("fusion"));
    const ThemeColors& colors = currentThemeColors();
    const QPalette palette = QApplication::palette();
    QCOMPARE(palette.color(QPalette::Window), colors.space);
    QCOMPARE(palette.color(QPalette::Base), colors.field);
    QCOMPARE(palette.color(QPalette::Text), colors.text);
    QCOMPARE(palette.color(QPalette::Highlight), colors.accent);
    QCOMPARE(palette.color(QPalette::HighlightedText), colors.onAccent);
    QCOMPARE(palette.color(QPalette::Disabled, QPalette::Text), colors.disabledText);
}

void TestTheme::titleFont()
{
    QFont font(QStringLiteral("Arial"), 11);
    const QFont title = ::titleFont(font);
    QVERIFY(title.bold());
    QCOMPARE(title.pointSize(), 11);
    // Its own family when Space Grotesk could not be loaded.
    QCOMPARE(title.families(), (QStringList{"Space Grotesk", "Arial"}));
}

void TestTheme::icon_data()
{
    QTest::addColumn<GlossaryTerm>("term");

    QTest::newRow("Universe") << GlossaryTerm::Universe;
    QTest::newRow("Galaxy") << GlossaryTerm::Galaxy;
    QTest::newRow("Astre") << GlossaryTerm::Astre;
    QTest::newRow("Comet") << GlossaryTerm::Comet;
}

void TestTheme::icon()
{
    QFETCH(GlossaryTerm, term);

    const QIcon icon = ::icon(term);
    QVERIFY(!icon.isNull());
    // Drawn by the icon engine of Qt Svg.
    QVERIFY(!icon.pixmap(32).isNull());
}

void TestTheme::logo()
{
    QVERIFY(QFile::exists(logoPath()));
}

QTEST_MAIN(TestTheme)
#include "TestTheme.moc"
