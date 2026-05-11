#include <QApplication>
#include <QDebug>
#include "MainWindow.h"
#include "DeviceController.h"

/**
 * @brief Point d'entrée de l'application
 *
 * Initialise l'application Qt et démarre l'interface de pilotage STM32.
 * Architecture MVC avec communication asynchrone par threading.
 *
 * @param argc Nombre d'arguments
 * @param argv Arguments de la ligne de commande
 * @return Code de sortie de l'application
 */
int main(int argc, char *argv[])
{
    // Configuration de l'application
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

    // Création du contrôleur principal (architecture MVC)
    DeviceController *controller = new DeviceController(&app);

    // Création de la fenêtre principale (View)
    MainWindow window(controller);
    window.show();

    qDebug() << "Application started successfully";
    qDebug() << "Main thread ID:" << QThread::currentThreadId();

    // Démarrage de la boucle d'événements Qt
    int result = app.exec();

    qDebug() << "Application exiting with code:" << result;

    return result;
}
