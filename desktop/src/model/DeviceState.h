#ifndef DEVICESTATE_H
#define DEVICESTATE_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>

/**
 * @brief Classe représentant l'état complet du dispositif STM32
 * 
 * Cette classe fait partie du modèle (Model) dans l'architecture MVC.
 * Elle encapsule toutes les données d'état du microcontrôleur.
 */
class DeviceState : public QObject
{
    Q_OBJECT
    
    // Propriétés Q_PROPERTY pour QML binding
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)
    Q_PROPERTY(float temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(float voltage READ voltage WRITE setVoltage NOTIFY voltageChanged)
    Q_PROPERTY(uint16_t adcRaw READ adcRaw WRITE setAdcRaw NOTIFY adcRawChanged)
    Q_PROPERTY(uint8_t pwmDutyCycle READ pwmDutyCycle WRITE setPwmDutyCycle NOTIFY pwmDutyCycleChanged)
    Q_PROPERTY(bool ledState READ ledState WRITE setLedState NOTIFY ledStateChanged)
    Q_PROPERTY(quint32 uptime READ uptime WRITE setUptime NOTIFY uptimeChanged)
    Q_PROPERTY(quint32 rxCharCount READ rxCharCount WRITE setRxCharCount NOTIFY rxCharCountChanged)
    Q_PROPERTY(QString firmwareVersion READ firmwareVersion WRITE setFirmwareVersion NOTIFY firmwareVersionChanged)

public:
    explicit DeviceState(QObject *parent = nullptr);
    
    // Getters
    bool isConnected() const { return m_connected; }
    float temperature() const { return m_temperature; }
    float voltage() const { return m_voltage; }
    uint16_t adcRaw() const { return m_adcRaw; }
    uint8_t pwmDutyCycle() const { return m_pwmDutyCycle; }
    bool ledState() const { return m_ledState; }
    quint32 uptime() const { return m_uptime; }
    quint32 rxCharCount() const { return m_rxCharCount; }
    QString firmwareVersion() const { return m_firmwareVersion; }
    QDateTime lastUpdate() const { return m_lastUpdate; }
    
    // Setters
    void setConnected(bool connected);
    void setTemperature(float temp);
    void setVoltage(float voltage);
    void setAdcRaw(uint16_t raw);
    void setPwmDutyCycle(uint8_t duty);
    void setLedState(bool state);
    void setUptime(quint32 uptime);
    void setRxCharCount(quint32 count);
    void setFirmwareVersion(const QString &version);
    
    // Méthodes utilitaires
    void reset();
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    QString toString() const;

signals:
    void connectedChanged(bool connected);
    void temperatureChanged(float temperature);
    void voltageChanged(float voltage);
    void adcRawChanged(uint16_t raw);
    void pwmDutyCycleChanged(uint8_t duty);
    void ledStateChanged(bool state);
    void uptimeChanged(quint32 uptime);
    void rxCharCountChanged(quint32 count);
    void firmwareVersionChanged(const QString &version);
    void stateUpdated();

private:
    bool m_connected;
    float m_temperature;
    float m_voltage;
    uint16_t m_adcRaw;
    uint8_t m_pwmDutyCycle;
    bool m_ledState;
    quint32 m_uptime;
    quint32 m_rxCharCount;
    QString m_firmwareVersion;
    QDateTime m_lastUpdate;
};

#endif // DEVICESTATE_H
