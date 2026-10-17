// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QFont>

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
};

// The colors of scheme; the dark ones when the system does not tell its scheme.
[[nodiscard]] const ThemeColors& themeColors(Qt::ColorScheme scheme);

// Gives the application the style that paints everything in the colors of its palette, the font of the titles, and the
// palette of the color scheme of the system, then again each time that scheme changes. Called once, after the
// QApplication is created and before any widget is.
void applyTheme();

// font, in the family of the titles, Space Grotesk, bold: for the name of what the edition zone shows, and the one of
// NebulaStudio. Its own family when Space Grotesk could not be loaded.
[[nodiscard]] QFont titleFont(const QFont& font);
