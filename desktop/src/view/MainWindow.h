#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QMessageBox>
#include <functional>
#include "DeviceController.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(DeviceController *controller, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onRefreshPorts();
    void onAuthenticateClicked();
    void onSendClicked();
    void onGetTempClicked();
    void onGetVoltageClicked();
    void onGetStatusClicked();
    void onLedOnClicked();
    void onLedOffClicked();
    void onPwmSliderChanged(int value);
    void onPwmSliderReleased();
    void onResetClicked();
    void onClearMonitor();
    void onClearSecurityLogs();
    void onExportSecurityLogs();
    void onExportLogs();
    void onSecurityReport();
    void onConnectionChanged(bool connected);
    void onDataReceived(const QString &data);
    void onTemperatureUpdated(float temperature);
    void onVoltageUpdated(float voltage);
    void onStatusUpdated(const QJsonObject &status);
    void onDeviceError(const QString &error);
    void updateSessionInfo();

private:
    Ui::MainWindow *ui;
    DeviceController *m_controller;

    bool m_isAuthenticated;
    QDateTime m_sessionStartTime;

    struct SecurityStats {
        int failedAuthAttempts;
        int malformedPackets;
        int replayAttempts;
        int successfulCommands;
    };
    SecurityStats m_securityStats;

    void setupUI();
    void setupConnections();
    void setupSecurityMonitoring();
    void setupSecurityStatsTable();
    void loadSerialPorts();
    void updateConnectionState(bool connected);
    void updateAuthenticationState(bool authenticated);
    void updateSecurityStats();
    void updateSecurityLevel();
    int getSecurityLevel();
    QString getSessionDuration();
    QString getUptime();
    void logSecurityEvent(const QString &event);
    bool checkAuthentication();
    void showStyledMessageBox(const QString &title, const QString &message, QMessageBox::Icon icon);
    void showStyledQuestion(const QString &title, const QString &message,
                           std::function<void()> onYes = nullptr,
                           std::function<void()> onNo = nullptr);
    void initializeSecurityCards();
};

#endif
