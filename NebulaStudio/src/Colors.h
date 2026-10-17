// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <algorithm>
#include <cmath>

// color as it looks over background, which is opaque: opaque too.
[[nodiscard]] inline QColor over(const QColor& color, const QColor& background)
{
    const float alpha = color.alphaF();
    return QColor::fromRgbF(color.redF() * alpha + background.redF() * (1 - alpha),
                            color.greenF() * alpha + background.greenF() * (1 - alpha),
                            color.blueF() * alpha + background.blueF() * (1 - alpha));
}

// The relative luminance of color, which is opaque, as WCAG defines it: from 0 for black to 1 for white.
[[nodiscard]] inline qreal luminance(const QColor& color)
{
    const auto linear = [](qreal channel)
    { return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4); };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

// The contrast ratio of two opaque colors as WCAG defines it, from 1 to 21. Its level AA asks 4.5 for a text, and 3
// for a large one and for the edges of what can be used.
[[nodiscard]] inline qreal contrast(const QColor& first, const QColor& second)
{
    const auto [darker, lighter] = std::minmax(luminance(first), luminance(second));
    return (lighter + 0.05) / (darker + 0.05);
}

// Black or white, whichever contrasts more with color, which is opaque: 4.58 at least, enough for WCAG AA.
[[nodiscard]] inline QColor textColorOver(const QColor& color)
{
    return contrast(color, Qt::white) >= contrast(color, Qt::black) ? Qt::white : Qt::black;
}
