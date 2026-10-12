#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    MainWindow window;
    window.show();
    // A project file given on the command line is opened at once.
    if(const QStringList arguments = QApplication::arguments(); arguments.size() > 1)
    {
        window.openProjectFile(arguments.at(1));
    }
    return QApplication::exec();
}
