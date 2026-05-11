#include "DeviceController.h"
#include <QDebug>
#include <QJsonDocument>

DeviceController::DeviceController(QObject *parent)
    : QObject(parent)
    , m_autoRefreshEnabled(false)
{
    // Initialisation des modèles
    m_deviceState = new DeviceState(this);
    m_dataModel = new DataModel(this);
    
    // Initialisation de la communication
    m_serialManager = new SerialManager(this);
    m_jsonProtocol = new JsonProtocol(this);
    
    // Timer pour rafraîchissement automatique
    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout,
            this, &DeviceController::handleAutoRefreshTimeout);
    
    // === CONNEXIONS SERIALMANAGER ===
    connect(m_serialManager, &SerialManager::dataReceived,
            this, &DeviceController::handleDataReceived);
    
    connect(m_serialManager, &SerialManager::connectionStatusChanged,
            this, &DeviceController::handleConnectionChanged);
    
    connect(m_serialManager, &SerialManager::errorOccurred,
            this, &DeviceController::handleSerialError);
    
    qDebug() << "[DeviceController] Initialized with MVC architecture";
}

DeviceController::~DeviceController()
{
    disconnectFromDevice();
    qDebug() << "[DeviceController] Destroyed";
}

bool DeviceController::isConnected() const
{
    return m_serialManager->isConnected();
}

bool DeviceController::connectToDevice(const QString &portName, qint32 baudRate)
{
    qDebug() << "[DeviceController] Connecting to" << portName << "@" << baudRate;
    return m_serialManager->openPort(portName, baudRate);
}

void DeviceController::disconnectFromDevice()
{
    qDebug() << "[DeviceController] Disconnecting";
    
    // Arrête le rafraîchissement automatique
    if (m_autoRefreshTimer->isActive()) {
        m_autoRefreshTimer->stop();
    }
    
    m_serialManager->closePort();
}

void DeviceController::setLed(bool state)
{
    qDebug() << "[DeviceController] Setting LED to" << (state ? "ON" : "OFF");
    
    QByteArray command = JsonProtocol::encodeSetLed(state);
    m_serialManager->sendCommand(command);
    
    // Mise à jour immédiate du modèle (sera confirmé par la réponse)
    m_deviceState->setLedState(state);
    
    emit commandSent(state ? "SET_LED=1" : "SET_LED=0");
}

void DeviceController::toggleLed()
{
    bool currentState = m_deviceState->ledState();
    setLed(!currentState);
}

void DeviceController::setPwm(uint8_t dutyCycle)
{
    if (dutyCycle > 100) {
        dutyCycle = 100;
    }
    
    qDebug() << "[DeviceController] Setting PWM to" << dutyCycle << "%";
    
    QByteArray command = JsonProtocol::encodeSetPwm(dutyCycle);
    m_serialManager->sendCommand(command);
    
    // Mise à jour immédiate du modèle
    m_deviceState->setPwmDutyCycle(dutyCycle);
    m_dataModel->addPwmPoint(dutyCycle);
    
    emit commandSent(QString("SET_PWM=%1").arg(dutyCycle));
}

void DeviceController::resetDevice()
{
    qDebug() << "[DeviceController] Resetting device";
    
    QByteArray command = JsonProtocol::encodeReset();
    m_serialManager->sendCommandPriority(command);  // Priorité haute
    
    emit commandSent("RESET");
}

void DeviceController::requestTemperature()
{
    QByteArray command = JsonProtocol::encodeGetTemperature();
    m_serialManager->sendCommand(command);
    emit commandSent("GET_TEMP");
}

void DeviceController::requestVoltage()
{
    QByteArray command = JsonProtocol::encodeGetVoltage();
    m_serialManager->sendCommand(command);
    emit commandSent("GET_VOLTAGE");
}

void DeviceController::requestAdcRaw()
{
    // Envoi d'une commande personnalisée
    sendCustomCommand("GET_ADC_RAW");
}

void DeviceController::requestStatus()
{
    QByteArray command = JsonProtocol::encodeGetStatus();
    m_serialManager->sendCommand(command);
    emit commandSent("STATUS");
}

void DeviceController::setHeartbeatInterval(uint32_t intervalMs)
{
    qDebug() << "[DeviceController] Setting heartbeat interval to" << intervalMs << "ms";
    
    QJsonObject params;
    params["interval"] = static_cast<qint64>(intervalMs);
    
    QByteArray command = JsonProtocol::encodeCommand("SET_HEARTBEAT", params);
    m_serialManager->sendCommand(command);
}

void DeviceController::setAutoRefresh(bool enabled, uint32_t intervalMs)
{
    m_autoRefreshEnabled = enabled;
    
    if (enabled) {
        qDebug() << "[DeviceController] Auto-refresh enabled, interval:" << intervalMs << "ms";
        m_autoRefreshTimer->start(intervalMs);
    } else {
        qDebug() << "[DeviceController] Auto-refresh disabled";
        m_autoRefreshTimer->stop();
    }
}

void DeviceController::sendCustomCommand(const QString &command)
{
    qDebug() << "[DeviceController] Sending custom command:" << command;
    
    QByteArray data = command.toUtf8();
    if (!data.endsWith('\n')) {
        data.append('\n');
    }
    
    m_serialManager->sendCommand(data);
    emit commandSent(command);
}

void DeviceController::sendJsonCommand(const QJsonObject &json)
{
    QByteArray data = JsonProtocol::formatJsonForSerial(json);
    m_serialManager->sendCommand(data);
    
    QString command = json["command"].toString();
    emit commandSent(command);
}

void DeviceController::handleDataReceived(const QByteArray &data)
{
    qDebug() << "[DeviceController] Data received:" << data.left(100);
    parseResponse(data);
}

void DeviceController::handleConnectionChanged(bool connected)
{
    qDebug() << "[DeviceController] Connection status changed:" << connected;
    
    m_deviceState->setConnected(connected);
    emit connectedChanged(connected);
    
    if (connected && m_autoRefreshEnabled) {
        m_autoRefreshTimer->start();
    } else {
        m_autoRefreshTimer->stop();
    }
}

void DeviceController::handleSerialError(const QString &error)
{
    qDebug() << "[DeviceController] Serial error:" << error;
    emit deviceError(error);
}

void DeviceController::handleAutoRefreshTimeout()
{
    // Demande périodique de l'état complet
    requestStatus();
}

void DeviceController::parseResponse(const QByteArray &data)
{
    // Tente de parser en JSON d'abord
    if (JsonProtocol::isValidJson(data)) {
        bool ok;
        QJsonObject json = JsonProtocol::parseMessage(data, &ok);
        
        if (ok) {
            parseJsonResponse(json);
            return;
        }
    }
    
    // Sinon, parse comme texte brut
    QString text = QString::fromUtf8(data);
    parseTextResponse(text);
}

void DeviceController::parseJsonResponse(const QJsonObject &json)
{
    JsonProtocol::MessageType type = JsonProtocol::getMessageType(
        QJsonDocument(json).toJson()
    );
    
    qDebug() << "[DeviceController] JSON message type:" << JsonProtocol::messageTypeToString(type);
    
    switch (type) {
        case JsonProtocol::Response:
            updateTemperatureFromJson(json);
            updateVoltageFromJson(json);
            updateStatusFromJson(json);
            break;
            
        case JsonProtocol::Heartbeat:
            updateHeartbeatFromJson(json);
            emit heartbeatReceived();
            break;
            
        case JsonProtocol::Error:
            {
                QString errorMsg;
                if (JsonProtocol::extractError(json, &errorMsg)) {
                    emit deviceError(errorMsg);
                }
            }
            break;
            
        default:
            break;
    }
    
    emit responseReceived(QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void DeviceController::parseTextResponse(const QString &text)
{
    qDebug() << "[DeviceController] Parsing text response:" << text;
    
    // Parse des réponses texte simples
    if (text.startsWith("TEMP:")) {
        QString tempStr = text.mid(5).trimmed();
        tempStr.remove("°C");
        bool ok;
        float temp = tempStr.toFloat(&ok);
        if (ok) {
            m_deviceState->setTemperature(temp);
            m_dataModel->addTemperaturePoint(temp);
            emit temperatureUpdated(temp);
        }
    }
    else if (text.startsWith("VOLTAGE:")) {
        QString voltStr = text.split(' ')[1].trimmed();
        voltStr.remove("V");
        bool ok;
        float volt = voltStr.toFloat(&ok);
        if (ok) {
            m_deviceState->setVoltage(volt);
            m_dataModel->addVoltagePoint(volt);
            emit voltageUpdated(volt);
        }
    }
    
    emit responseReceived(text);
}

void DeviceController::updateTemperatureFromJson(const QJsonObject &json)
{
    float temperature;
    if (JsonProtocol::extractTemperature(json, &temperature)) {
        m_deviceState->setTemperature(temperature);
        m_dataModel->addTemperaturePoint(temperature);
        emit temperatureUpdated(temperature);
    }
}

void DeviceController::updateVoltageFromJson(const QJsonObject &json)
{
    float voltage;
    uint16_t adcRaw;
    if (JsonProtocol::extractVoltage(json, &voltage, &adcRaw)) {
        m_deviceState->setVoltage(voltage);
        m_deviceState->setAdcRaw(adcRaw);
        m_dataModel->addVoltagePoint(voltage);
        emit voltageUpdated(voltage);
    }
}

void DeviceController::updateStatusFromJson(const QJsonObject &json)
{
    QJsonObject status;
    if (JsonProtocol::extractStatus(json, &status)) {
        // Mise à jour de tous les paramètres d'état
        if (status.contains("temp")) {
            m_deviceState->setTemperature(status["temp"].toDouble());
        }
        if (status.contains("voltage")) {
            m_deviceState->setVoltage(status["voltage"].toDouble());
        }
        if (status.contains("led")) {
            m_deviceState->setLedState(status["led"].toBool());
        }
        if (status.contains("pwm")) {
            m_deviceState->setPwmDutyCycle(status["pwm"].toInt());
        }
        if (status.contains("uptime")) {
            m_deviceState->setUptime(status["uptime"].toInt());
        }
        
        emit statusUpdated(status);
    }
}

void DeviceController::updateHeartbeatFromJson(const QJsonObject &json)
{
    // Traite les données de heartbeat
    if (json.contains("data") && json["data"].isObject()) {
        QJsonObject data = json["data"].toObject();
        
        if (data.contains("rx_chars")) {
            m_deviceState->setRxCharCount(data["rx_chars"].toInt());
        }
        if (data.contains("temp")) {
            m_deviceState->setTemperature(data["temp"].toDouble());
        }
        if (data.contains("pwm")) {
            m_deviceState->setPwmDutyCycle(data["pwm"].toInt());
        }
    }
}
