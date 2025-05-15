#include <QApplication>
#include "viewer/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();
    mainWindow.loadDefaultModel();

    return app.exec();
}
