// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QCommandLineParser>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption pluginRoot(
        QStringLiteral("plugin-root"),
        QCoreApplication::translate("main", "Directory the files of the plugins are relative to in a project file "
                                            "(default: the directory of Forge)."),
        QCoreApplication::translate("main", "directory"));
    parser.addOption(pluginRoot);
    parser.addPositionalArgument(QStringLiteral("project"),
                                 QCoreApplication::translate("main", "Project file to open."),
                                 QStringLiteral("[project]"));
    parser.process(app);

    MainWindow window(parser.value(pluginRoot));
    window.show();
    // A project file given on the command line is opened at once.
    if(const QStringList projects = parser.positionalArguments(); !projects.isEmpty())
    {
        window.openProjectFile(projects.constFirst());
    }
    return QApplication::exec();
}
