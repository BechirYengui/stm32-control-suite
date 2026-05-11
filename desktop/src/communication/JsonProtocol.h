#ifndef JSONPROTOCOL_H
#define JSONPROTOCOL_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QString>

/**
 * @brief Protocole de communication JSON pour échanges structurés
 * 
 * Encode et décode les messages en format JSON pour une communication
 * robuste et extensible avec le microcontrôleur.
 * 
 * Format des messages:
 * - Commande: {"type":"cmd", "command":"GET_TEMP", "params":{}}
 * - Réponse: {"type":"response", "data":{"temp":25.5}, "status":"ok"}
 * - Erreur: {"type":"error", "message":"Invalid command"}
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
    
    // Encodage des messages
    static QByteArray encodeCommand(const QString &command, const QJsonObject &params = QJsonObject());
    static QByteArray encodeSetLed(bool state);
    static QByteArray encodeSetPwm(uint8_t dutyCycle);
    static QByteArray encodeGetStatus();
    static QByteArray encodeGetTemperature();
    static QByteArray encodeGetVoltage();
    static QByteArray encodeReset();
    
    // Décodage des messages
    static MessageType getMessageType(const QByteArray &data);
    static QJsonObject parseMessage(const QByteArray &data, bool *ok = nullptr);
    static bool isValidJson(const QByteArray &data);
    
    // Extraction de données spécifiques
    static bool extractTemperature(const QJsonObject &json, float *temperature);
    static bool extractVoltage(const QJsonObject &json, float *voltage, uint16_t *adcRaw);
    static bool extractStatus(const QJsonObject &json, QJsonObject *status);
    static bool extractError(const QJsonObject &json, QString *errorMessage);
    
    // Utilitaires
    static QString messageTypeToString(MessageType type);
    static QByteArray formatJsonForSerial(const QJsonObject &json);

signals:
    void parseError(const QString &error);

private:
    static QJsonObject createBaseMessage(const QString &type);
};

#endif // JSONPROTOCOL_H
