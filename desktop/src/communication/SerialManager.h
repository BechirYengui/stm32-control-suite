#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QThread>
#include <QSerialPortInfo>
#include "SerialWorker.h"

/**
 * @brief Gestionnaire de communication série avec threading
 * 
 * Cette classe gère la communication série en utilisant un worker dans
 * un thread séparé pour éviter de bloquer l'interface utilisateur.
 * 
 * Architecture:
 * - SerialManager (thread principal) : Interface de haut niveau
 * - SerialWorker (thread dédié) : Opérations I/O série
 */
class SerialManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();
    
    // Gestion de la connexion
    bool openPort(const QString &portName, qint32 baudRate = 115200);
    void closePort();
    bool isConnected() const { return m_connected; }
    
    // Envoi de données
    bool sendCommand(const QByteArray &command);
    bool sendCommandPriority(const QByteArray &command);
    
    // Informations
    QString getPortName() const { return m_portName; }
    qint32 getBaudRate() const { return m_baudRate; }
    QString getPortInfo() const;
    
    // Utilitaires statiques
    static QStringList availablePorts();
    static QString getPortDescription(const QString &portName);

signals:
    // Signaux de communication
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &error);
    
    // Statistiques
    void bytesWritten(qint64 bytes);
    void bytesReceived(qint64 bytes);
    
    // Signaux internes pour le worker (via queued connections)
    void requestOpenPort(const QString &portName, qint32 baudRate);
    void requestClosePort();
    void requestSendData(const QByteArray &data);
    void requestSendDataPriority(const QByteArray &data);

private slots:
    void handlePortOpened(const QString &portName, qint32 baudRate);
    void handlePortClosed();
    void handleOpenError(const QString &error);
    void handleWorkerError(const QString &error);

private:
    void setupWorkerThread();
    void cleanupWorkerThread();
    
    SerialWorker *m_worker;
    QThread *m_workerThread;
    
    QString m_portName;
    qint32 m_baudRate;
    bool m_connected;
};

#endif // SERIALMANAGER_H
