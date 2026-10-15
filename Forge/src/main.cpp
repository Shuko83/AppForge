// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

#include "MainWindow.h"
#include "SplashScreen.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/forge.png"))); // Of all its windows, and on the taskbar

    QCommandLineParser parser;
    parser.addHelpOption();
    AppForge::ForgeApplication::addCommandLineOptions(parser);
    parser.addPositionalArgument(QStringLiteral("project"),
                                 QCoreApplication::translate("main", "Project file to open."),
                                 QStringLiteral("[project]"));
    parser.process(app);

    // Shown while the plugins are scanned, then until it is read.
    SplashScreen splash;
    splash.show();
    // Painted before the scan; a click waits for it to end, when read() can be received.
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    MainWindow window(&parser);
    const QStringList projects = parser.positionalArguments();
    QObject::connect(&splash, &SplashScreen::read, &window,
                     [&splash, &window, &projects]
                     {
                         window.show();
                         splash.finish(&window);
                         // A project file given on the command line is opened as soon as the window shows.
                         if(!projects.isEmpty())
                         {
                             window.openProjectFile(projects.constFirst());
                         }
                     });
    return QApplication::exec();
}
