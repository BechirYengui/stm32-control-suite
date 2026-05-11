#include <QtTest/QtTest>
#include <QSignalSpy>
#include "DeviceState.h"

class TestDeviceState : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testTemperatureThreshold();
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

QTEST_GUILESS_MAIN(TestDeviceState)
#include "test_devicestate.moc"
