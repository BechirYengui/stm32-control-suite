#include <QApplication>
#include <QDebug>
#include <cstring>
#include "MainWindow.h"
#include "DeviceController.h"

#ifdef BUILD_WITH_QML
#include <QQmlApplicationEngine>
#include <QQmlContext>
#endif

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

    bool useQml = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--qml") == 0) {
            useQml = true;
            break;
        }
    }

    DeviceController *controller = new DeviceController(&app);

#ifdef BUILD_WITH_QML
    QQmlApplicationEngine engine;
    MainWindow *window = nullptr;

    if (useQml) {
        engine.rootContext()->setContextProperty("deviceState", controller->deviceState());
        engine.load(QUrl("qrc:/main.qml"));
        if (engine.rootObjects().isEmpty()) {
            qWarning() << "Failed to load QML, falling back to widgets";
            window = new MainWindow(controller);
            window->show();
        }
    } else {
        window = new MainWindow(controller);
        window->show();
    }
#else
    if (useQml) {
        qWarning() << "QML mode requested but not built; falling back to widgets";
    }
    MainWindow window(controller);
    window.show();
#endif

    qDebug() << "Application started successfully";
    qDebug() << "Main thread ID:" << QThread::currentThreadId();

    int result = app.exec();

    qDebug() << "Application exiting with code:" << result;

    return result;
}
