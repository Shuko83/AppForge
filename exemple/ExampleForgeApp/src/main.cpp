#include <QApplication>
#include <QTextStream>

#include "AppForge/Application/ForgeApplication.h"

// ExampleForgeApp <project file>: the application Forge saved there, running until its last window is closed.
int main(int argc, char* argv[])
{
    // A QApplication rather than a QCoreApplication: the components of the project may show widgets.
    const QApplication app(argc, argv);
    QTextStream err(stderr);
    if(QApplication::arguments().size() != 2)
    {
        err << "Usage: ExampleForgeApp <project file>\n";
        return 1;
    }

    // open() loads the plugins of the project, instantiates its components, sets their properties and binds their
    // interfaces; start() initializes and starts them. The application stops them when it is destroyed.
    AppForge::ForgeApplication application;
    if(!application.open(QApplication::arguments().at(1)) || !application.start())
    {
        err << application.errorString() << '\n';
        return 1;
    }
    return QApplication::exec();
}
