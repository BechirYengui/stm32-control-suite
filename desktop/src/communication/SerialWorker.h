#ifndef SERIALWORKER_H
#define SERIALWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

/**
 * Asynchronous serial-port worker. Runs on its own QThread so I/O never
 * blocks the UI thread; outbound writes are queued and flushed in order.
 */
class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

    bool isRunning() const { return m_running; }
    QString portName() const { return m_portName; }
    qint32 baudRate() const { return m_baudRate; }

public slots:
    void openPort(const QString &portName, qint32 baudRate);
    void closePort();

    void sendData(const QByteArray &data);
    void sendDataPriority(const QByteArray &data);

    void start();
    void stop();

signals:
    void portOpened(const QString &portName, qint32 baudRate);
    void portClosed();
    void openError(const QString &error);
    void dataReceived(const QByteArray &data);
    void dataSent(const QByteArray &data);
    void errorOccurred(const QString &error);
    void bytesWritten(qint64 bytes);
    void bytesReceived(qint64 bytes);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void processSendQueue();

private:
    void setupSerialPort();
    void cleanupSerialPort();
    bool sendDataInternal(const QByteArray &data);
    
    QSerialPort *m_serialPort;
    QString m_portName;
    qint32 m_baudRate;
    
    QByteArray m_receiveBuffer;
    QQueue<QByteArray> m_sendQueue;
    
    QMutex m_queueMutex;
    QMutex m_portMutex;
    QWaitCondition m_queueCondition;
    
    bool m_running;
    bool m_stopRequested;
    
    static constexpr int BUFFER_SIZE = 8192;
    static constexpr int MAX_QUEUE_SIZE = 100;
    
    quint64 m_totalBytesSent;
    quint64 m_totalBytesReceived;
};

#endif // SERIALWORKER_H
