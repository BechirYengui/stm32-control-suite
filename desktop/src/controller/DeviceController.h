#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include <QTimer>
#include "DeviceState.h"
#include "DataModel.h"
#include "SerialManager.h"
#include "JsonProtocol.h"

/**
 * @brief Contrôleur principal du dispositif STM32 (MVC Controller)
 * 
 * Cette classe implémente le contrôleur dans l'architecture MVC:
 * - Gère la logique métier
 * - Coordonne le modèle (DeviceState, DataModel) et la vue
 * - Interface avec la couche communication (SerialManager)
 * - Parse les données JSON reçues
 * - Met à jour le modèle de données
 */
class DeviceController : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(DeviceState* deviceState READ deviceState CONSTANT)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController();
    
    // Accès aux modèles
    DeviceState* deviceState() const { return m_deviceState; }
    DataModel* dataModel() const { return m_dataModel; }
    SerialManager* serialManager() const { return m_serialManager; }
    
    // État de connexion
    bool isConnected() const;

public slots:
    // === COMMANDES DE CONNEXION ===
    bool connectToDevice(const QString &portName, qint32 baudRate = 115200);
    void disconnectFromDevice();
    
    // === COMMANDES DE CONTRÔLE ===
    void setLed(bool state);
    void toggleLed();
    void setPwm(uint8_t dutyCycle);
    void resetDevice();
    
    // === COMMANDES DE LECTURE ===
    void requestTemperature();
    void requestVoltage();
    void requestAdcRaw();
    void requestStatus();
    
    // === CONFIGURATION ===
    void setHeartbeatInterval(uint32_t intervalMs);
    void setAutoRefresh(bool enabled, uint32_t intervalMs = 1000);
    
    // === COMMANDES AVANCÉES ===
    void sendCustomCommand(const QString &command);
    void sendJsonCommand(const QJsonObject &json);

signals:
    // Notifications de changement d'état
    void connectedChanged(bool connected);
    void deviceError(const QString &error);
    void commandSent(const QString &command);
    void responseReceived(const QString &response);
    
    // Événements spécifiques
    void temperatureUpdated(float temperature);
    void voltageUpdated(float voltage);
    void statusUpdated(const QJsonObject &status);
    void heartbeatReceived();

private slots:
    // Gestion des données reçues
    void handleDataReceived(const QByteArray &data);
    void handleConnectionChanged(bool connected);
    void handleSerialError(const QString &error);
    
    // Rafraîchissement automatique
    void handleAutoRefreshTimeout();

private:
    // Parsing des réponses
    void parseResponse(const QByteArray &data);
    void parseJsonResponse(const QJsonObject &json);
    void parseTextResponse(const QString &text);
    
    // Extraction de données spécifiques
    void updateTemperatureFromJson(const QJsonObject &json);
    void updateVoltageFromJson(const QJsonObject &json);
    void updateStatusFromJson(const QJsonObject &json);
    void updateHeartbeatFromJson(const QJsonObject &json);
    
    // Modèles (Model dans MVC)
    DeviceState *m_deviceState;
    DataModel *m_dataModel;
    
    // Communication
    SerialManager *m_serialManager;
    JsonProtocol *m_jsonProtocol;
    
    // Rafraîchissement automatique
    QTimer *m_autoRefreshTimer;
    bool m_autoRefreshEnabled;
};

#endif // DEVICECONTROLLER_H
