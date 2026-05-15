#include <QtTest/QtTest>
#include <QSignalSpy>
#include "DeviceState.h"

class TestDeviceState : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testTemperatureThreshold();
    void testPwmClamp();
    void testFormattedUptime();
};

void TestDeviceState::testDefaults()
{
    DeviceState s;
    QCOMPARE(s.isConnected(), false);
    QCOMPARE(s.temperature(), 0.0f);
    QCOMPARE(s.ledState(), false);
    QCOMPARE(s.firmwareVersion(), QString("Unknown"));
}

void TestDeviceState::testTemperatureThreshold()
{
    DeviceState s;
    QSignalSpy spy(&s, &DeviceState::temperatureChanged);

    s.setTemperature(25.0f);
    s.setTemperature(25.005f); // sous le seuil 0.01, doit etre ignore
    s.setTemperature(30.0f);

    QCOMPARE(spy.count(), 2);
}

void TestDeviceState::testPwmClamp()
{
    DeviceState s;
    s.setPwmDutyCycle(200); // au dessus du max, doit etre clamp a 100
    QCOMPARE(s.pwmDutyCycle(), static_cast<uint8_t>(DeviceState::kMaxPwmDutyCycle));

    s.setPwmDutyCycle(50);
    QCOMPARE(s.pwmDutyCycle(), static_cast<uint8_t>(50));
}

void TestDeviceState::testFormattedUptime()
{
    DeviceState s;
    s.setUptime(0);
    QCOMPARE(s.formattedUptime(), QString("00:00:00"));

    s.setUptime(65); // 1 min 5 s
    QCOMPARE(s.formattedUptime(), QString("00:01:05"));

    s.setUptime(3661); // 1h 1min 1s
    QCOMPARE(s.formattedUptime(), QString("01:01:01"));
}

QTEST_GUILESS_MAIN(TestDeviceState)
#include "test_devicestate.moc"
