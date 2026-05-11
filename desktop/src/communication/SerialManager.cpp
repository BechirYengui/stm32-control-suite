#include "SerialManager.h"
#include <QDebug>

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
    , m_baudRate(115200)
    , m_connected(false)
{
    setupWorkerThread();
    qDebug() << "[SerialManager] Initialized with threaded architecture";
}

SerialManager::~SerialManager()
{
    closePort();
    cleanupWorkerThread();
    qDebug() << "[SerialManager] Destroyed";
}

void SerialManager::setupWorkerThread()
{
    // Crée le thread worker
    m_workerThread = new QThread(this);
    m_worker = new SerialWorker();
    
    // Déplace le worker dans son thread
    m_worker->moveToThread(m_workerThread);
    
    // === CONNEXIONS POUR GESTION DU THREAD ===
    connect(m_workerThread, &QThread::started,
            m_worker, &SerialWorker::start);
    
    connect(m_workerThread, &QThread::finished,
            m_worker, &SerialWorker::deleteLater);
    
    // === CONNEXIONS POUR LES COMMANDES (Queued pour cross-thread) ===
    connect(this, &SerialManager::requestOpenPort,
            m_worker, &SerialWorker::openPort, Qt::QueuedConnection);
    
    connect(this, &SerialManager::requestClosePort,
            m_worker, &SerialWorker::closePort, Qt::QueuedConnection);
    
    connect(this, &SerialManager::requestSendData,
            m_worker, &SerialWorker::sendData, Qt::QueuedConnection);
    
    connect(this, &SerialManager::requestSendDataPriority,
            m_worker, &SerialWorker::sendDataPriority, Qt::QueuedConnection);
    
    // === CONNEXIONS POUR LES ÉVÉNEMENTS DU WORKER ===
    connect(m_worker, &SerialWorker::portOpened,
            this, &SerialManager::handlePortOpened);
    
    connect(m_worker, &SerialWorker::portClosed,
            this, &SerialManager::handlePortClosed);
    
    connect(m_worker, &SerialWorker::openError,
            this, &SerialManager::handleOpenError);
    
    connect(m_worker, &SerialWorker::dataReceived,
            this, &SerialManager::dataReceived);
    
    connect(m_worker, &SerialWorker::dataSent,
            this, &SerialManager::dataSent);
    
    connect(m_worker, &SerialWorker::errorOccurred,
            this, &SerialManager::handleWorkerError);
    
    connect(m_worker, &SerialWorker::bytesWritten,
            this, &SerialManager::bytesWritten);
    
    connect(m_worker, &SerialWorker::bytesReceived,
            this, &SerialManager::bytesReceived);
    
    // Démarre le thread
    m_workerThread->start();
    qDebug() << "[SerialManager] Worker thread started";
}

void SerialManager::cleanupWorkerThread()
{
    if (m_workerThread) {
        qDebug() << "[SerialManager] Stopping worker thread...";
        
        // Arrête le worker
        if (m_worker) {
            m_worker->stop();
        }
        
        // Arrête et attend la fin du thread
        m_workerThread->quit();
        if (!m_workerThread->wait(3000)) {
            qDebug() << "[SerialManager] WARNING: Thread did not finish, terminating";
            m_workerThread->terminate();
            m_workerThread->wait();
        }
        
        qDebug() << "[SerialManager] Worker thread stopped";
    }
}

bool SerialManager::openPort(const QString &portName, qint32 baudRate)
{
    if (m_connected) {
        qDebug() << "[SerialManager] Already connected, closing first";
        closePort();
    }
    
    m_portName = portName;
    m_baudRate = baudRate;
    
    qDebug() << "[SerialManager] Requesting port open:" << portName << "@" << baudRate;
    emit requestOpenPort(portName, baudRate);
    
    // Retourne true immédiatement, la confirmation viendra via signal
    return true;
}

void SerialManager::closePort()
{
    if (!m_connected) {
        return;
    }
    
    qDebug() << "[SerialManager] Requesting port close";
    emit requestClosePort();
}

bool SerialManager::sendCommand(const QByteArray &command)
{
    if (!m_connected) {
        emit errorOccurred("Port not connected");
        return false;
    }
    
    emit requestSendData(command);
    return true;
}

bool SerialManager::sendCommandPriority(const QByteArray &command)
{
    if (!m_connected) {
        emit errorOccurred("Port not connected");
        return false;
    }
    
    emit requestSendDataPriority(command);
    return true;
}

QString SerialManager::getPortInfo() const
{
    if (!m_connected) {
        return "Port closed";
    }
    
    return QString("Port: %1 | Baudrate: %2 | Status: Connected")
        .arg(m_portName)
        .arg(m_baudRate);
}

QStringList SerialManager::availablePorts()
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    
    for (const QSerialPortInfo &info : infos) {
        QString portName = info.portName();
        
        // Filtre les ports série legacy (ttyS0-31)
        if (portName.startsWith("ttyS")) {
            continue;
        }
        
        ports << portName;
    }
    
    qDebug() << "[SerialManager] Available ports:" << ports;
    return ports;
}

QString SerialManager::getPortDescription(const QString &portName)
{
    const auto infos = QSerialPortInfo::availablePorts();
    
    for (const QSerialPortInfo &info : infos) {
        if (info.portName() == portName) {
            QString desc = info.description();
            if (desc.isEmpty()) {
                desc = "Serial Port";
            }
            return desc;
        }
    }
    
    return "Unknown";
}

void SerialManager::handlePortOpened(const QString &portName, qint32 baudRate)
{
    m_connected = true;
    m_portName = portName;
    m_baudRate = baudRate;
    
    emit connectionStatusChanged(true);
    qDebug() << "[SerialManager] Port opened successfully:" << portName << "@" << baudRate;
}

void SerialManager::handlePortClosed()
{
    m_connected = false;
    emit connectionStatusChanged(false);
    qDebug() << "[SerialManager] Port closed";
}

void SerialManager::handleOpenError(const QString &error)
{
    m_connected = false;
    emit errorOccurred("Open error: " + error);
    qDebug() << "[SerialManager] Open error:" << error;
}

void SerialManager::handleWorkerError(const QString &error)
{
    emit errorOccurred(error);
    qDebug() << "[SerialManager] Worker error:" << error;
}
