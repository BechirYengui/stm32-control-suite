#include <QApplication>
#include <QDebug>
#include "MainWindow.h"
#include "DeviceController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("IMT Atlantique");
    app.setOrganizationDomain("imt-atlantique.fr");
    app.setApplicationName("STM32 Interface");
    app.setApplicationVersion("1.0.0");

    qDebug() << "========================================";
    qDebug() << "  STM32 Interface - IMT Atlantique";
    qDebug() << "  Version:" << app.applicationVersion();
    qDebug() << "========================================";
    qDebug() << "Architecture: MVC with QThread";
    qDebug() << "Communication: JSON over Serial + DMA";
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "========================================";

    DeviceController *controller = new DeviceController(&app);

    MainWindow window(controller);
    window.show();

    qDebug() << "Application started successfully";
    qDebug() << "Main thread ID:" << QThread::currentThreadId();

    int result = app.exec();

    qDebug() << "Application exiting with code:" << result;

    return result;
}
