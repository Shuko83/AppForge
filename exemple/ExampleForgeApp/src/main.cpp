// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QCommandLineParser>
#include <QTextStream>

#include "AppForge/Application/ForgeApplication.h"

// ExampleForgeApp [--plugin-dir <directory>]... [--plugin-root <directory>] <project file>: the application Forge saved
// there, running until its last window is closed.
int main(int argc, char* argv[])
{
    // A QApplication rather than a QCoreApplication: the components of the project may show widgets.
    const QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Runs the application of a project file, as Forge saved it, until its last window is closed."));
    parser.addHelpOption();
    AppForge::ForgeApplication::addCommandLineOptions(parser);
    parser.addPositionalArgument(QStringLiteral("project"), QStringLiteral("Project file to run."));
    parser.process(app);
    const QStringList projects = parser.positionalArguments();
    if(projects.size() != 1)
    {
        parser.showHelp(1);
    }

    // The plugins of the directories of the command line are found before the project looks for its own. open() loads
    // the plugins of the project, instantiates its components, sets their properties and binds their interfaces;
    // start() starts them. The application stops them when it is destroyed.
    AppForge::ForgeApplication application;
    application.applyCommandLine(parser);
    if(!application.open(projects.constFirst()) || !application.start())
    {
        QTextStream(stderr) << application.errorString() << '\n';
        return 1;
    }
    return QApplication::exec();
}
