// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "SplashScreen.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <chrono>
#include <string_view>

#include "NebulaStudio_info.h"
#include "Theme.h"

namespace
{

// How long it stays when it is not clicked.
constexpr auto ReadingTime = std::chrono::seconds(2);
// Its width, and the space around what it says, in device-independent pixels.
constexpr int Width = 480;
constexpr int Margin = 24;
// The size of the name of the framework, relative to the font of the application.
constexpr qreal TitleScale = 2.5;

QString fromInfo(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

// When NebulaStudio was built, in the format of the locale; as the compiler wrote it when it cannot be read.
QString buildDateText()
{
    const QString text = fromInfo(NebulaStudioInfo::buildDate).simplified();
    const QDateTime buildDate = QLocale::c().toDateTime(text, QStringLiteral("MMM d yyyy HH:mm:ss"));
    return buildDate.isValid() ? QLocale().toString(buildDate, QLocale::ShortFormat) : text;
}

// The name of the framework on a band of the highlight color, then the rest of what it says, under it, at the
// resolution of the screens.
QPixmap picture()
{
    const QPalette palette = QApplication::palette();
    const QFont font = QApplication::font();
    QFont nameFont = titleFont(font);
    nameFont.setPointSizeF(font.pointSizeF() * TitleScale);
    const int titleHeight = QFontMetrics(nameFont).height();
    const int lineHeight = QFontMetrics(font).lineSpacing();

    // Under the band, one per line; an empty one leaves a line's space.
    const QStringList lines = {
        SplashScreen::tr("Version %1").arg(fromInfo(NebulaStudioInfo::version)),
        SplashScreen::tr("Built with Qt %1 and %2")
            .arg(fromInfo(NebulaStudioInfo::qtVersion), fromInfo(NebulaStudioInfo::compiler)),
        SplashScreen::tr("Built on %1").arg(buildDateText()),
        QString(),
        fromInfo(NebulaStudioInfo::copyright),
        SplashScreen::tr("Licensed under the Mozilla Public License 2.0"),
    };

    const int bandHeight = 2 * Margin + titleHeight + lineHeight;
    const QSize size(Width, bandHeight + 2 * Margin + static_cast<int>(lines.size()) * lineHeight);
    const qreal ratio = qApp->devicePixelRatio();
    QPixmap pixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(palette.color(QPalette::Window));

    QPainter painter(&pixmap);
    painter.fillRect(0, 0, Width, bandHeight, palette.color(QPalette::Highlight));
    painter.setPen(palette.color(QPalette::Mid));
    painter.drawRect(QRect(QPoint(), size).adjusted(0, 0, -1, -1)); // Its edges, whatever is behind it

    // Draws text in the current font and pen, on the line of height whose top is y; returns the top of the next one.
    const auto drawLine = [&painter](int y, int height, const QString& text)
    {
        constexpr int TextWidth = Width - 2 * Margin;
        painter.drawText(QRect(Margin, y, TextWidth, height), Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(text, Qt::ElideRight, TextWidth));
        return y + height;
    };
    painter.setPen(palette.color(QPalette::HighlightedText));
    painter.setFont(nameFont);
    const int taglineTop = drawLine(Margin, titleHeight, fromInfo(NebulaStudioInfo::product));
    painter.setFont(font);
    drawLine(taglineTop, lineHeight, SplashScreen::tr("Build modular applications by assembling components"));

    painter.setPen(palette.color(QPalette::WindowText));
    int top = bandHeight + Margin;
    for(const QString& line : lines)
    {
        top = drawLine(top, lineHeight, line);
    }
    return pixmap;
}

} // namespace

SplashScreen::SplashScreen() : QSplashScreen(picture())
{
    m_readingTimer.setSingleShot(true);
    m_readingTimer.setInterval(ReadingTime);
    connect(&m_readingTimer, &QTimer::timeout, this, &SplashScreen::setRead);
}

void SplashScreen::showEvent(QShowEvent* event)
{
    QSplashScreen::showEvent(event);
    if(!m_read && !m_readingTimer.isActive())
    {
        m_readingTimer.start();
    }
}

void SplashScreen::mousePressEvent(QMouseEvent* /*event*/)
{
    setRead();
}

void SplashScreen::setRead()
{
    if(m_read)
    {
        return;
    }
    m_read = true;
    m_readingTimer.stop();
    emit read();
}
