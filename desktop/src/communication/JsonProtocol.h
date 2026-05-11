#ifndef JSONPROTOCOL_H
#define JSONPROTOCOL_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QString>

/**
 * JSON wire-format codec for STM32 communication.
 * Wire format:
 *   - Command:  {"type":"cmd",      "command":"GET_TEMP", "params":{}}
 *   - Response: {"type":"response", "data":{"temp":25.5}, "status":"ok"}
 *   - Error:    {"type":"error",    "message":"Invalid command"}
 */
class JsonProtocol : public QObject
{
    Q_OBJECT

public:
    enum MessageType {
        Command,
        Response,
        Error,
        Heartbeat,
        Unknown
    };

    explicit JsonProtocol(QObject *parent = nullptr);

    static QByteArray encodeCommand(const QString &command, const QJsonObject &params = QJsonObject());
    static QByteArray encodeSetLed(bool state);
    static QByteArray encodeSetPwm(uint8_t dutyCycle);
    static QByteArray encodeGetStatus();
    static QByteArray encodeGetTemperature();
    static QByteArray encodeGetVoltage();
    static QByteArray encodeReset();

    static MessageType getMessageType(const QByteArray &data);
    static QJsonObject parseMessage(const QByteArray &data, bool *ok = nullptr);
    static bool isValidJson(const QByteArray &data);

    static bool extractTemperature(const QJsonObject &json, float *temperature);
    static bool extractVoltage(const QJsonObject &json, float *voltage, uint16_t *adcRaw);
    static bool extractStatus(const QJsonObject &json, QJsonObject *status);
    static bool extractError(const QJsonObject &json, QString *errorMessage);

    static QString messageTypeToString(MessageType type);
    static QByteArray formatJsonForSerial(const QJsonObject &json);

signals:
    void parseError(const QString &error);

private:
    static QJsonObject createBaseMessage(const QString &type);
};

#endif // JSONPROTOCOL_H
