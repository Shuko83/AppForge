#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return QApplication::exec();
}
