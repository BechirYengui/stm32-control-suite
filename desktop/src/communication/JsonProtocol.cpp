#include "JsonProtocol.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

JsonProtocol::JsonProtocol(QObject *parent)
    : QObject(parent)
{
}

QJsonObject JsonProtocol::createBaseMessage(const QString &type)
{
    QJsonObject message;
    message["type"] = type;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return message;
}

QByteArray JsonProtocol::encodeCommand(const QString &command, const QJsonObject &params)
{
    QJsonObject message = createBaseMessage("cmd");
    message["command"] = command;

    if (!params.isEmpty()) {
        message["params"] = params;
    }

    return formatJsonForSerial(message);
}

QByteArray JsonProtocol::encodeSetLed(bool state)
{
    QJsonObject params;
    params["state"] = state ? 1 : 0;
    return encodeCommand("SET_LED", params);
}

QByteArray JsonProtocol::encodeSetPwm(uint8_t dutyCycle)
{
    QJsonObject params;
    params["duty"] = dutyCycle;
    return encodeCommand("SET_PWM", params);
}

QByteArray JsonProtocol::encodeGetStatus()
{
    return encodeCommand("STATUS");
}

QByteArray JsonProtocol::encodeGetTemperature()
{
    return encodeCommand("GET_TEMP");
}

QByteArray JsonProtocol::encodeGetVoltage()
{
    return encodeCommand("GET_VOLTAGE");
}

QByteArray JsonProtocol::encodeReset()
{
    return encodeCommand("RESET");
}

JsonProtocol::MessageType JsonProtocol::getMessageType(const QByteArray &data)
{
    bool ok;
    QJsonObject json = parseMessage(data, &ok);

    if (!ok) {
        return Unknown;
    }

    QString type = json["type"].toString();

    if (type == "response") return Response;
    if (type == "cmd" || type == "command") return Command;
    if (type == "error") return Error;
    if (type == "heartbeat") return Heartbeat;

    return Unknown;
}

QJsonObject JsonProtocol::parseMessage(const QByteArray &data, bool *ok)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (ok) *ok = false;
        qDebug() << "[JsonProtocol] Parse error:" << parseError.errorString();
        return QJsonObject();
    }

    if (!doc.isObject()) {
        if (ok) *ok = false;
        qDebug() << "[JsonProtocol] Document is not an object";
        return QJsonObject();
    }

    if (ok) *ok = true;
    return doc.object();
}

bool JsonProtocol::isValidJson(const QByteArray &data)
{
    QJsonParseError parseError;
    QJsonDocument::fromJson(data, &parseError);
    return (parseError.error == QJsonParseError::NoError);
}

bool JsonProtocol::extractTemperature(const QJsonObject &json, float *temperature)
{
    if (!json.contains("data") || !json["data"].isObject()) {
        return false;
    }

    QJsonObject data = json["data"].toObject();

    if (!data.contains("temp") && !data.contains("temperature")) {
        return false;
    }

    if (temperature) {
        if (data.contains("temp")) {
            *temperature = static_cast<float>(data["temp"].toDouble());
        } else {
            *temperature = static_cast<float>(data["temperature"].toDouble());
        }
    }

    return true;
}

bool JsonProtocol::extractVoltage(const QJsonObject &json, float *voltage, uint16_t *adcRaw)
{
    if (!json.contains("data") || !json["data"].isObject()) {
        return false;
    }

    QJsonObject data = json["data"].toObject();

    bool hasVoltage = data.contains("voltage") || data.contains("volt");

    if (!hasVoltage) {
        return false;
    }

    if (voltage) {
        if (data.contains("voltage")) {
            *voltage = static_cast<float>(data["voltage"].toDouble());
        } else {
            *voltage = static_cast<float>(data["volt"].toDouble());
        }
    }

    if (adcRaw && data.contains("adc_raw")) {
        *adcRaw = static_cast<uint16_t>(data["adc_raw"].toInt());
    }

    return true;
}

bool JsonProtocol::extractStatus(const QJsonObject &json, QJsonObject *status)
{
    if (!json.contains("data") || !json["data"].isObject()) {
        return false;
    }

    if (status) {
        *status = json["data"].toObject();
    }

    return true;
}

bool JsonProtocol::extractError(const QJsonObject &json, QString *errorMessage)
{
    if (json["type"].toString() != "error") {
        return false;
    }

    if (errorMessage) {
        *errorMessage = json["message"].toString();
    }

    return true;
}

QString JsonProtocol::messageTypeToString(MessageType type)
{
    switch (type) {
        case Command: return "Command";
        case Response: return "Response";
        case Error: return "Error";
        case Heartbeat: return "Heartbeat";
        case Unknown: return "Unknown";
        default: return "Invalid";
    }
}

QByteArray JsonProtocol::formatJsonForSerial(const QJsonObject &json)
{
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    // Ajoute un retour à la ligne pour délimiter les messages
    data.append('\n');

    return data;
}
