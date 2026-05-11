#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include <QTimer>
#include "DeviceState.h"
#include "DataModel.h"
#include "SerialManager.h"
#include "JsonProtocol.h"

/**
 * MVC controller for the STM32 device. Coordinates DeviceState/DataModel
 * with the SerialManager, parses inbound JSON and text responses, and
 * exposes command slots to the views.
 */
class DeviceController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(DeviceState* deviceState READ deviceState CONSTANT)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController();

    DeviceState* deviceState() const { return m_deviceState; }
    DataModel* dataModel() const { return m_dataModel; }
    SerialManager* serialManager() const { return m_serialManager; }

    bool isConnected() const;

public slots:
    bool connectToDevice(const QString &portName, qint32 baudRate = 115200);
    void disconnectFromDevice();

    void setLed(bool state);
    void toggleLed();
    void setPwm(uint8_t dutyCycle);
    void resetDevice();

    void requestTemperature();
    void requestVoltage();
    void requestAdcRaw();
    void requestStatus();

    void setHeartbeatInterval(uint32_t intervalMs);
    void setAutoRefresh(bool enabled, uint32_t intervalMs = 1000);

    void sendCustomCommand(const QString &command);
    void sendJsonCommand(const QJsonObject &json);

signals:
    void connectedChanged(bool connected);
    void deviceError(const QString &error);
    void commandSent(const QString &command);
    void responseReceived(const QString &response);

    void temperatureUpdated(float temperature);
    void voltageUpdated(float voltage);
    void statusUpdated(const QJsonObject &status);
    void heartbeatReceived();

private slots:
    void handleDataReceived(const QByteArray &data);
    void handleConnectionChanged(bool connected);
    void handleSerialError(const QString &error);

    void handleAutoRefreshTimeout();

private:
    void parseResponse(const QByteArray &data);
    void parseJsonResponse(const QJsonObject &json);
    void parseTextResponse(const QString &text);

    void updateTemperatureFromJson(const QJsonObject &json);
    void updateVoltageFromJson(const QJsonObject &json);
    void updateStatusFromJson(const QJsonObject &json);
    void updateHeartbeatFromJson(const QJsonObject &json);

    DeviceState *m_deviceState;
    DataModel *m_dataModel;

    SerialManager *m_serialManager;
    JsonProtocol *m_jsonProtocol;

    QTimer *m_autoRefreshTimer;
    bool m_autoRefreshEnabled;
};

#endif // DEVICECONTROLLER_H
