#include <QtTest/QtTest>
#include <QJsonDocument>
#include "JsonProtocol.h"

class TestJsonProtocol : public QObject
{
    Q_OBJECT

private slots:
    void testEncodeSetLed();
    void testGetMessageType();
    void testExtractTemperature();
};

void TestJsonProtocol::testEncodeSetLed()
{
    QByteArray data = JsonProtocol::encodeSetLed(true);
    QVERIFY(data.endsWith('\n'));

    QJsonObject obj = JsonProtocol::parseMessage(data);
    QCOMPARE(obj["type"].toString(), QString("cmd"));
    QCOMPARE(obj["command"].toString(), QString("SET_LED"));
    QCOMPARE(obj["params"].toObject()["state"].toInt(), 1);
}

void TestJsonProtocol::testGetMessageType()
{
    QByteArray cmd  = R"({"type":"cmd","command":"STATUS"})";
    QByteArray resp = R"({"type":"response","data":{}})";
    QByteArray err  = R"({"type":"error","message":"oops"})";

    QCOMPARE(JsonProtocol::getMessageType(cmd),  JsonProtocol::Command);
    QCOMPARE(JsonProtocol::getMessageType(resp), JsonProtocol::Response);
    QCOMPARE(JsonProtocol::getMessageType(err),  JsonProtocol::Error);

    // edge case: pas du JSON
    QCOMPARE(JsonProtocol::getMessageType("not json"), JsonProtocol::Unknown);
}

void TestJsonProtocol::testExtractTemperature()
{
    QJsonObject ok = QJsonDocument::fromJson(R"({"data":{"temp":25.5}})").object();
    float t = 0.0f;
    QVERIFY(JsonProtocol::extractTemperature(ok, &t));
    QVERIFY(qAbs(t - 25.5f) < 0.001f);

    QJsonObject alt = QJsonDocument::fromJson(R"({"data":{"temperature":30.0}})").object();
    QVERIFY(JsonProtocol::extractTemperature(alt, &t));
    QVERIFY(qAbs(t - 30.0f) < 0.001f);

    QJsonObject bad = QJsonDocument::fromJson(R"({"data":{}})").object();
    QVERIFY(!JsonProtocol::extractTemperature(bad, &t));
}

QTEST_APPLESS_MAIN(TestJsonProtocol)
#include "test_jsonprotocol.moc"
