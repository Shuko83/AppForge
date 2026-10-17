// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPalette>
#include <QStyleHints>

namespace
{

// The family of Space Grotesk, as its file names it.
const QString TitleFamily = QStringLiteral("Space Grotesk");

// The night sky: the accent is light, and what is written over it dark, for the accent to stand out from the fields
// as much as its text does from it.
const ThemeColors Dark = {
    .space = QColor(0x0D0B1E),
    .field = QColor(0x15122B),
    .alternateField = QColor(0x1D1939),
    .surface = QColor(0x262046),
    .edge = QColor(0x7A6FB0),
    .text = QColor(0xEEEAFE),
    .mutedText = QColor(0xB3AAD9),
    .disabledText = QColor(0x6F6797),
    .accent = QColor(0xA78BFA),
    .onAccent = QColor(0x0D0B1E),
    .link = QColor(0x8AB4FF),
    .visitedLink = QColor(0xF49AD3),
};

// Its dawn.
const ThemeColors Light = {
    .space = QColor(0xF4F2FC),
    .field = QColor(0xFFFFFF),
    .alternateField = QColor(0xF0ECFB),
    .surface = QColor(0xE8E3F8),
    .edge = QColor(0x8579B8),
    .text = QColor(0x1A1530),
    .mutedText = QColor(0x574F7A),
    .disabledText = QColor(0xA39CBF),
    .accent = QColor(0x6D28D9),
    .onAccent = QColor(0xFFFFFF),
    .link = QColor(0x1D4ED8),
    .visitedLink = QColor(0xA21C6B),
};

// The roles of the palette in the colors of a scheme, the same whether its window is active or not.
QPalette palette(const ThemeColors& colors)
{
    // How much the bevels of the style are lighter or darker than the surface, in percent.
    constexpr int BevelFactor = 130;

    QPalette palette;
    palette.setColor(QPalette::Window, colors.space);
    palette.setColor(QPalette::WindowText, colors.text);
    palette.setColor(QPalette::Base, colors.field);
    palette.setColor(QPalette::AlternateBase, colors.alternateField);
    palette.setColor(QPalette::Text, colors.text);
    palette.setColor(QPalette::PlaceholderText, colors.mutedText);
    palette.setColor(QPalette::Button, colors.surface);
    palette.setColor(QPalette::ButtonText, colors.text);
    palette.setColor(QPalette::BrightText, colors.text);
    palette.setColor(QPalette::ToolTipBase, colors.surface);
    palette.setColor(QPalette::ToolTipText, colors.text);
    palette.setColor(QPalette::Light, colors.surface.lighter(BevelFactor));
    palette.setColor(QPalette::Midlight, colors.surface.lighter((100 + BevelFactor) / 2));
    palette.setColor(QPalette::Mid, colors.edge);
    palette.setColor(QPalette::Dark, colors.edge.darker(BevelFactor));
    palette.setColor(QPalette::Shadow, colors.space.darker(BevelFactor));
    palette.setColor(QPalette::Highlight, colors.accent);
    palette.setColor(QPalette::HighlightedText, colors.onAccent);
    palette.setColor(QPalette::Accent, colors.accent);
    palette.setColor(QPalette::Link, colors.link);
    palette.setColor(QPalette::LinkVisited, colors.visitedLink);

    for(const QPalette::ColorRole role :
        {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::PlaceholderText})
    {
        palette.setColor(QPalette::Disabled, role, colors.disabledText);
    }
    palette.setColor(QPalette::Disabled, QPalette::Highlight, colors.edge);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, colors.text);
    return palette;
}

void applyColorScheme(Qt::ColorScheme scheme)
{
    QApplication::setPalette(palette(themeColors(scheme)));
}

} // namespace

const ThemeColors& themeColors(Qt::ColorScheme scheme)
{
    return scheme == Qt::ColorScheme::Light ? Light : Dark;
}

void applyTheme()
{
    // The one style of Qt that paints all it draws in the colors of the palette, on every platform.
    QApplication::setStyle(QStringLiteral("Fusion"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/SpaceGrotesk-Bold.ttf"));

    QStyleHints* hints = QGuiApplication::styleHints();
    applyColorScheme(hints->colorScheme());
    QObject::connect(hints, &QStyleHints::colorSchemeChanged, qApp, &applyColorScheme);
}

QFont titleFont(const QFont& font)
{
    QFont title = font;
    title.setFamilies({TitleFamily, font.family()}); // The second one when the first could not be loaded
    title.setBold(true);
    return title;
}

QIcon icon(GlossaryTerm term)
{
    switch(term)
    {
    case GlossaryTerm::Universe:
        return QIcon(QStringLiteral(":/icons/universe.svg"));
    case GlossaryTerm::Galaxy:
        return QIcon(QStringLiteral(":/icons/galaxy.svg"));
    case GlossaryTerm::Astre:
        return QIcon(QStringLiteral(":/icons/astre.svg"));
    case GlossaryTerm::Comet:
        return QIcon(QStringLiteral(":/icons/comet.svg"));
    }
    return {};
}

QString logoPath()
{
    return QStringLiteral(":/nebula.svg");
}
