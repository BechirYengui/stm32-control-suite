#include "SerialWorker.h"
#include <QDebug>
#include <QThread>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
    , m_serialPort(nullptr)
    , m_baudRate(115200)
    , m_running(false)
    , m_stopRequested(false)
    , m_totalBytesSent(0)
    , m_totalBytesReceived(0)
{
    m_receiveBuffer.reserve(BUFFER_SIZE);
    qDebug() << "[SerialWorker] Initialized in thread" << QThread::currentThreadId();
}

SerialWorker::~SerialWorker()
{
    stop();
    cleanupSerialPort();
    qDebug() << "[SerialWorker] Destroyed";
}

void SerialWorker::start()
{
    qDebug() << "[SerialWorker] Starting in thread" << QThread::currentThreadId();
    m_running = true;
    m_stopRequested = false;
}

void SerialWorker::stop()
{
    qDebug() << "[SerialWorker] Stop requested";
    m_stopRequested = true;
    m_running = false;

    // Réveille le thread s'il attend
    m_queueCondition.wakeAll();
}

void SerialWorker::setupSerialPort()
{
    if (m_serialPort) {
        cleanupSerialPort();
    }

    m_serialPort = new QSerialPort(this);

    connect(m_serialPort, &QSerialPort::readyRead,
            this, &SerialWorker::handleReadyRead);

    // ✅ CORRECTION: Utilise errorOccurred au lieu de error (Qt 5.8+)
    connect(m_serialPort, &QSerialPort::errorOccurred,
            this, &SerialWorker::handleError);
}

void SerialWorker::cleanupSerialPort()
{
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        m_serialPort->deleteLater();
        m_serialPort = nullptr;
    }
}

void SerialWorker::openPort(const QString &portName, qint32 baudRate)
{
    QMutexLocker locker(&m_portMutex);

    qDebug() << "[SerialWorker] Opening port" << portName << "@" << baudRate << "bauds";

    // Ferme le port existant
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    setupSerialPort();

    m_portName = portName;
    m_baudRate = baudRate;

    // Configuration du port série
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Tentative d'ouverture
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_receiveBuffer.clear();
        m_totalBytesSent = 0;
        m_totalBytesReceived = 0;

        emit portOpened(portName, baudRate);
        qDebug() << "[SerialWorker] Port opened successfully";
    } else {
        QString errorMsg = "Failed to open " + portName + ": " + m_serialPort->errorString();
        emit openError(errorMsg);
        qDebug() << "[SerialWorker] ERROR:" << errorMsg;
    }
}

void SerialWorker::closePort()
{
    QMutexLocker locker(&m_portMutex);

    qDebug() << "[SerialWorker] Closing port";

    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
        m_receiveBuffer.clear();

        // Vide la queue d'envoi
        QMutexLocker queueLocker(&m_queueMutex);
        m_sendQueue.clear();

        emit portClosed();
        qDebug() << "[SerialWorker] Port closed";
    }
}

void SerialWorker::sendData(const QByteArray &data)
{
    QMutexLocker locker(&m_queueMutex);

    if (m_sendQueue.size() >= MAX_QUEUE_SIZE) {
        qDebug() << "[SerialWorker] WARNING: Send queue full, dropping message";
        emit errorOccurred("Send queue overflow");
        return;
    }

    m_sendQueue.enqueue(data);
    qDebug() << "[SerialWorker] Data queued, queue size:" << m_sendQueue.size();

    // Traite la queue immédiatement
    processSendQueue();
}

void SerialWorker::sendDataPriority(const QByteArray &data)
{
    QMutexLocker locker(&m_queueMutex);

    // Insère en tête de queue pour priorité
    m_sendQueue.prepend(data);
    qDebug() << "[SerialWorker] Priority data queued, queue size:" << m_sendQueue.size();

    processSendQueue();
}

void SerialWorker::processSendQueue()
{
    // Ne prend pas le mutex ici car appelé depuis une fonction qui l'a déjà

    while (!m_sendQueue.isEmpty()) {
        QByteArray data = m_sendQueue.dequeue();

        if (!sendDataInternal(data)) {
            qDebug() << "[SerialWorker] Failed to send data, requeuing";
            m_sendQueue.prepend(data);
            break;
        }
    }
}

bool SerialWorker::sendDataInternal(const QByteArray &data)
{
    QMutexLocker locker(&m_portMutex);

    if (!m_serialPort || !m_serialPort->isOpen()) {
        emit errorOccurred("Port not connected");
        return false;
    }

    qint64 written = m_serialPort->write(data);

    if (written == -1) {
        QString errorMsg = "Write error: " + m_serialPort->errorString();
        emit errorOccurred(errorMsg);
        qDebug() << "[SerialWorker] ERROR:" << errorMsg;
        return false;
    }

    // Force l'envoi immédiat
    if (!m_serialPort->flush()) {
        qDebug() << "[SerialWorker] WARNING: Flush failed";
    }

    m_totalBytesSent += written;

    emit dataSent(data);
    emit bytesWritten(written);

    qDebug() << "[SerialWorker] TX:" << written << "bytes -" << data.toHex(' ').left(60);

    return true;
}

void SerialWorker::handleReadyRead()
{
    QMutexLocker locker(&m_portMutex);

    if (!m_serialPort || !m_serialPort->isOpen()) {
        return;
    }

    // Lit toutes les données disponibles
    QByteArray data = m_serialPort->readAll();

    if (data.isEmpty()) {
        return;
    }

    m_totalBytesReceived += data.size();

    qDebug() << "[SerialWorker] RX:" << data.size() << "bytes -" << data.toHex(' ').left(60);

    emit bytesReceived(data.size());

    // Ajoute au buffer de réception
    m_receiveBuffer.append(data);

    // Traite les lignes complètes (délimitées par '\n')
    while (m_receiveBuffer.contains('\n')) {
        int idx = m_receiveBuffer.indexOf('\n');
        QByteArray line = m_receiveBuffer.left(idx);
        m_receiveBuffer.remove(0, idx + 1);

        // Supprime '\r' si présent
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        // Émet uniquement les lignes non vides
        if (!line.isEmpty()) {
            emit dataReceived(line);
        }
    }

    // Protection contre le débordement de buffer
    if (m_receiveBuffer.size() > BUFFER_SIZE) {
        qDebug() << "[SerialWorker] ERROR: Buffer overflow, purging";
        m_receiveBuffer.clear();
        emit errorOccurred("Receive buffer overflow");
    }
}

void SerialWorker::handleError(QSerialPort::SerialPortError error)
{
    // Ignore les erreurs normales
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }

    QString errorMsg;

    switch (error) {
        case QSerialPort::DeviceNotFoundError:
            errorMsg = "Device not found";
            break;
        case QSerialPort::PermissionError:
            errorMsg = "Permission denied";
            break;
        case QSerialPort::OpenError:
            errorMsg = "Cannot open port";
            break;
        case QSerialPort::WriteError:
            errorMsg = "Write error";
            break;
        case QSerialPort::ReadError:
            errorMsg = "Read error";
            break;
        case QSerialPort::ResourceError:
            errorMsg = "Resource unavailable (device disconnected?)";
            // Ferme automatiquement en cas de déconnexion
            closePort();
            break;
        default:
            if (m_serialPort) {
                errorMsg = m_serialPort->errorString();
            }
            break;
    }

    qDebug() << "[SerialWorker] ERROR:" << errorMsg << "(code:" << error << ")";
    emit errorOccurred(errorMsg);
}
