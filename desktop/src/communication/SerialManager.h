#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QThread>
#include <QSerialPortInfo>
#include "SerialWorker.h"

/**
 * High-level facade for the serial link. Owns the SerialWorker and its
 * QThread; public methods are non-blocking and post requests to the
 * worker via queued connections.
 */
class SerialManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

    bool openPort(const QString &portName, qint32 baudRate = 115200);
    void closePort();
    bool isConnected() const { return m_connected; }

    bool sendCommand(const QByteArray &command);
    bool sendCommandPriority(const QByteArray &command);

    QString getPortName() const { return m_portName; }
    qint32 getBaudRate() const { return m_baudRate; }
    QString getPortInfo() const;

    static QStringList availablePorts();
    static QString getPortDescription(const QString &portName);

signals:
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &error);

    void bytesWritten(qint64 bytes);
    void bytesReceived(qint64 bytes);

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
