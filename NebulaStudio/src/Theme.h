// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QFont>
#include <QIcon>
#include <cstdint>

// The visual identity of NebulaStudio, on the theme of space: the night sky and the violets, blues and pinks of a
// nebula, in a dark scheme and in a light one, its dawn. Its colors are tokens named by what they are for, defined once
// here for each scheme, that applyTheme() gives to the roles of the palette: items and widgets take theirs from the
// palette. Each text and the background it is written on meet WCAG AA, 4.5 to 1 at least, and so do the edges of what
// can be used, 3 to 1, except what is disabled, which WCAG exempts.
struct ThemeColors
{
    QColor space;          // Behind the panels: the windows
    QColor field;          // Lists, fields, and the edition zone
    QColor alternateField; // Every other row, and the titles of the property editor
    QColor surface;        // Buttons, components and tooltips
    QColor edge;           // Outlines, separators and the grid
    QColor text;           // On any of the above
    QColor mutedText;      // Hints, and what qualifies a name: the component of an instance
    QColor disabledText;
    QColor accent;   // Selection and focus: the violet of the nebula
    QColor onAccent; // What is written over the accent
    QColor link;
    QColor visitedLink;
    // What is wrong and has to be fixed, a component that does not work: an amber, written on the surface and the
    // fields as a text is. The palette has no role for it: it is read from currentThemeColors() when painting.
    QColor warning;
    QColor onWarning; // What is written over the warning
};

// The colors of scheme; the dark ones when the system does not tell its scheme.
[[nodiscard]] const ThemeColors& themeColors(Qt::ColorScheme scheme);
// The colors of the scheme the system has now, those applyTheme() gave the palette.
[[nodiscard]] const ThemeColors& currentThemeColors();

// Gives the application the style that paints everything in the colors of its palette, the font of the titles, and the
// palette of the color scheme of the system, then again each time that scheme changes. Called once, after the
// QApplication is created and before any widget is.
void applyTheme();

// font, in the family of the titles, Space Grotesk, bold: for the name of what the edition zone shows, and the one of
// NebulaStudio. Its own family when Space Grotesk could not be loaded.
[[nodiscard]] QFont titleFont(const QFont& font);

// The terms of the glossary of Nebula (docs/glossary.md) that have an icon.
enum class GlossaryTerm : std::uint8_t
{
    Universe, // The application, the edition zone shows
    Galaxy,   // A zone
    Astre,    // A component, and its instances
    Comet,    // A settings file
};

// The icon of term, drawn in resources/icons/ in the colors of the zones of the theme, which read over the fields of
// both schemes. Its SVG is read by the icon engine of Qt Svg.
[[nodiscard]] QIcon icon(GlossaryTerm term);

// The path of the logo of Nebula in the resources, an SVG: the icon of the windows, and the one on the splash screen.
[[nodiscard]] QString logoPath();
