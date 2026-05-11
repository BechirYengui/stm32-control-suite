#include "DeviceState.h"
#include <QDebug>

DeviceState::DeviceState(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_temperature(0.0f)
    , m_voltage(0.0f)
    , m_adcRaw(0)
    , m_pwmDutyCycle(0)
    , m_ledState(false)
    , m_uptime(0)
    , m_rxCharCount(0)
    , m_firmwareVersion("Unknown")
{
    qDebug() << "[DeviceState] Initialized";
}

void DeviceState::setConnected(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        m_lastUpdate = QDateTime::currentDateTime();
        emit connectedChanged(connected);
        emit stateUpdated();
        
        if (!connected) {
            // Reset state on disconnection
            reset();
        }
    }
}

void DeviceState::setTemperature(float temp)
{
    if (qAbs(m_temperature - temp) > 0.01f) {
        m_temperature = temp;
        m_lastUpdate = QDateTime::currentDateTime();
        emit temperatureChanged(temp);
        emit stateUpdated();
    }
}

void DeviceState::setVoltage(float voltage)
{
    if (qAbs(m_voltage - voltage) > 0.001f) {
        m_voltage = voltage;
        m_lastUpdate = QDateTime::currentDateTime();
        emit voltageChanged(voltage);
        emit stateUpdated();
    }
}

void DeviceState::setAdcRaw(uint16_t raw)
{
    if (m_adcRaw != raw) {
        m_adcRaw = raw;
        m_lastUpdate = QDateTime::currentDateTime();
        emit adcRawChanged(raw);
        emit stateUpdated();
    }
}

void DeviceState::setPwmDutyCycle(uint8_t duty)
{
    if (m_pwmDutyCycle != duty) {
        m_pwmDutyCycle = duty;
        m_lastUpdate = QDateTime::currentDateTime();
        emit pwmDutyCycleChanged(duty);
        emit stateUpdated();
    }
}

void DeviceState::setLedState(bool state)
{
    if (m_ledState != state) {
        m_ledState = state;
        m_lastUpdate = QDateTime::currentDateTime();
        emit ledStateChanged(state);
        emit stateUpdated();
    }
}

void DeviceState::setUptime(quint32 uptime)
{
    if (m_uptime != uptime) {
        m_uptime = uptime;
        m_lastUpdate = QDateTime::currentDateTime();
        emit uptimeChanged(uptime);
        emit stateUpdated();
    }
}

void DeviceState::setRxCharCount(quint32 count)
{
    if (m_rxCharCount != count) {
        m_rxCharCount = count;
        m_lastUpdate = QDateTime::currentDateTime();
        emit rxCharCountChanged(count);
        emit stateUpdated();
    }
}

void DeviceState::setFirmwareVersion(const QString &version)
{
    if (m_firmwareVersion != version) {
        m_firmwareVersion = version;
        emit firmwareVersionChanged(version);
        emit stateUpdated();
    }
}

void DeviceState::reset()
{
    m_temperature = 0.0f;
    m_voltage = 0.0f;
    m_adcRaw = 0;
    m_pwmDutyCycle = 0;
    m_ledState = false;
    m_uptime = 0;
    m_rxCharCount = 0;
    
    emit temperatureChanged(m_temperature);
    emit voltageChanged(m_voltage);
    emit adcRawChanged(m_adcRaw);
    emit pwmDutyCycleChanged(m_pwmDutyCycle);
    emit ledStateChanged(m_ledState);
    emit uptimeChanged(m_uptime);
    emit rxCharCountChanged(m_rxCharCount);
    emit stateUpdated();
    
    qDebug() << "[DeviceState] Reset to default values";
}

QJsonObject DeviceState::toJson() const
{
    QJsonObject json;
    json["connected"] = m_connected;
    json["temperature"] = m_temperature;
    json["voltage"] = m_voltage;
    json["adc_raw"] = m_adcRaw;
    json["pwm_duty"] = m_pwmDutyCycle;
    json["led_state"] = m_ledState;
    json["uptime"] = static_cast<qint64>(m_uptime);
    json["rx_char_count"] = static_cast<qint64>(m_rxCharCount);
    json["firmware_version"] = m_firmwareVersion;
    json["last_update"] = m_lastUpdate.toString(Qt::ISODate);
    
    return json;
}

void DeviceState::fromJson(const QJsonObject &json)
{
    if (json.contains("temperature"))
        setTemperature(json["temperature"].toDouble());
    
    if (json.contains("voltage"))
        setVoltage(json["voltage"].toDouble());
    
    if (json.contains("adc_raw"))
        setAdcRaw(static_cast<uint16_t>(json["adc_raw"].toInt()));
    
    if (json.contains("pwm_duty"))
        setPwmDutyCycle(static_cast<uint8_t>(json["pwm_duty"].toInt()));
    
    if (json.contains("led_state"))
        setLedState(json["led_state"].toBool());
    
    if (json.contains("uptime"))
        setUptime(static_cast<quint32>(json["uptime"].toInt()));
    
    if (json.contains("rx_char_count"))
        setRxCharCount(static_cast<quint32>(json["rx_char_count"].toInt()));
    
    if (json.contains("firmware_version"))
        setFirmwareVersion(json["firmware_version"].toString());
}

QString DeviceState::toString() const
{
    return QString("DeviceState[Connected=%1, Temp=%.1f°C, Voltage=%.2fV, PWM=%2%%, LED=%3, Uptime=%4s]")
        .arg(m_connected ? "Yes" : "No")
        .arg(m_temperature, 0, 'f', 1)
        .arg(m_voltage, 0, 'f', 2)
        .arg(m_pwmDutyCycle)
        .arg(m_ledState ? "ON" : "OFF")
        .arg(m_uptime);
}
