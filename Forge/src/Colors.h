#pragma once

#include <QColor>

// color as it looks over background, which is opaque: opaque too.
[[nodiscard]] inline QColor over(const QColor& color, const QColor& background)
{
    const float alpha = color.alphaF();
    return QColor::fromRgbF(color.redF() * alpha + background.redF() * (1 - alpha),
                            color.greenF() * alpha + background.greenF() * (1 - alpha),
                            color.blueF() * alpha + background.blueF() * (1 - alpha));
}

// Black or white, whichever is read over color, which is opaque.
[[nodiscard]] inline QColor textColorOver(const QColor& color)
{
    // The gray above which a color is light.
    constexpr int LightGray = 128;
    return qGray(color.rgb()) < LightGray ? Qt::white : Qt::black;
}
