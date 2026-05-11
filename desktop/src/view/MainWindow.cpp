#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QTimer>
#include <QTableWidgetItem>
#include <QPushButton>
#include <functional>

MainWindow::MainWindow(DeviceController *controller, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(controller)
    , m_isAuthenticated(false)
    , m_sessionStartTime(QDateTime::currentDateTime())
{
    ui->setupUi(this);
    setupUI();
    setupConnections();
    setupSecurityMonitoring();
    qDebug() << "[MainWindow] Initialisé avec architecture sécurisée MVC";
}

MainWindow::~MainWindow()
{
    delete ui;
    qDebug() << "[MainWindow] Détruit";
}

void MainWindow::showStyledMessageBox(const QString &title, const QString &message, QMessageBox::Icon icon)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "   background-color: #16213e;"
        "   color: #ffffff;"
        "}"
        "QMessageBox QLabel {"
        "   color: #ffffff;"
        "   font-size: 11pt;"
        "   padding: 10px;"
        "   min-width: 300px;"
        "}"
        "QPushButton {"
        "   background-color: #0f3460;"
        "   color: #ffffff;"
        "   border: 2px solid #00d9ff;"
        "   padding: 8px 20px;"
        "   border-radius: 6px;"
        "   font-weight: bold;"
        "   min-width: 80px;"
        "   min-height: 35px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #00d9ff;"
        "   color: #000000;"
        "}"
    );
    msgBox.exec();
}

void MainWindow::showStyledQuestion(const QString &title, const QString &message,
                                     std::function<void()> onYes, std::function<void()> onNo)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    QPushButton *yesButton = msgBox.addButton("Oui", QMessageBox::YesRole);
    QPushButton *noButton = msgBox.addButton("Non", QMessageBox::NoRole);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "   background-color: #16213e;"
        "   color: #ffffff;"
        "}"
        "QMessageBox QLabel {"
        "   color: #ffffff;"
        "   font-size: 11pt;"
        "   padding: 10px;"
        "   min-width: 350px;"
        "}"
        "QPushButton {"
        "   background-color: #0f3460;"
        "   color: #ffffff;"
        "   border: 2px solid #00d9ff;"
        "   padding: 8px 20px;"
        "   border-radius: 6px;"
        "   font-weight: bold;"
        "   min-width: 80px;"
        "   min-height: 35px;"
        "}"
    );
    yesButton->setStyleSheet("background-color: #10b981; border-color: #10b981;");
    noButton->setStyleSheet("background-color: #ef4444; border-color: #ef4444;");
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton && onYes) {
        onYes();
    } else if (msgBox.clickedButton() == noButton && onNo) {
        onNo();
    }
}

void MainWindow::initializeSecurityCards()
{
    if (ui->failedAuthCountLabel) {
        ui->failedAuthCountLabel->setText("0");
    }
    if (ui->malformedPacketsCountLabel) {
        ui->malformedPacketsCountLabel->setText("0");
    }
    if (ui->successfulCommandsCountLabel) {
        ui->successfulCommandsCountLabel->setText("0");
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("STM32 SecureLink - Interface de Pilotage Sécurisé");
    ui->baudRateComboBox->setCurrentText("115200");
    loadSerialPorts();
    updateConnectionState(false);
    updateAuthenticationState(false);
    ui->statusbar->showMessage("⚫ Déconnecté - En attente de connexion", 0);
    ui->receiveTextEdit->setHtml(
        "<div style='color: #00d9ff; font-weight: bold;'>"
        "<pre>"
        "╔═══════════════════════════════════════════════════════════════╗\n"
        "║     🔐 STM32 SecureLink - Communication Sécurisée            ║\n"
        "╠═══════════════════════════════════════════════════════════════╣\n"
        "║  ✅ Chiffrement AES-128-CBC                                   ║\n"
        "║  ✅ Authentification HMAC-SHA256                              ║\n"
        "║  ✅ Protection Anti-Replay                                    ║\n"
        "║  ✅ Architecture MVC Moderne                                  ║\n"
        "╠═══════════════════════════════════════════════════════════════╣\n"
        "║  📌 Étapes:                                                   ║\n"
        "║     1. Connectez-vous au port série                          ║\n"
        "║     2. Authentifiez-vous (onglet Sécurité)                   ║\n"
        "║     3. Commencez à piloter le STM32                          ║\n"
        "╚═══════════════════════════════════════════════════════════════╝"
        "</pre>"
        "</div><br>"
    );
    setupSecurityStatsTable();
    ui->pwmValueLCD->display(50);
    initializeSecurityCards();
    ui->mainTabWidget->setCurrentIndex(0);
    qDebug() << "[MainWindow] Interface configurée";
}

void MainWindow::setupConnections()
{
    connect(m_controller, &DeviceController::connectedChanged, this, &MainWindow::onConnectionChanged);
    connect(m_controller, &DeviceController::responseReceived, this, &MainWindow::onDataReceived);
    connect(m_controller, &DeviceController::temperatureUpdated, this, &MainWindow::onTemperatureUpdated);
    connect(m_controller, &DeviceController::voltageUpdated, this, &MainWindow::onVoltageUpdated);
    connect(m_controller, &DeviceController::statusUpdated, this, &MainWindow::onStatusUpdated);
    connect(m_controller, &DeviceController::deviceError, this, &MainWindow::onDeviceError);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->refreshPortsButton, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    connect(ui->authenticateButton, &QPushButton::clicked, this, &MainWindow::onAuthenticateClicked);
    connect(ui->passwordLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onAuthenticateClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui->commandLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    connect(ui->getTempButton, &QPushButton::clicked, this, &MainWindow::onGetTempClicked);
    connect(ui->getVoltageButton, &QPushButton::clicked, this, &MainWindow::onGetVoltageClicked);
    connect(ui->getStatusButton, &QPushButton::clicked, this, &MainWindow::onGetStatusClicked);
    connect(ui->ledOnButton, &QPushButton::clicked, this, &MainWindow::onLedOnClicked);
    connect(ui->ledOffButton, &QPushButton::clicked, this, &MainWindow::onLedOffClicked);
    connect(ui->pwmSlider, &QSlider::valueChanged, this, &MainWindow::onPwmSliderChanged);
    connect(ui->pwmSlider, &QSlider::sliderReleased, this, &MainWindow::onPwmSliderReleased);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::onClearMonitor);
    connect(ui->clearSecLogsButton, &QPushButton::clicked, this, &MainWindow::onClearSecurityLogs);
    connect(ui->exportSecLogsButton, &QPushButton::clicked, this, &MainWindow::onExportSecurityLogs);
    connect(ui->actionExportLogs, &QAction::triggered, this, &MainWindow::onExportLogs);
    connect(ui->actionQuitter, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionAuthentifier, &QAction::triggered, this, &MainWindow::onAuthenticateClicked);
    connect(ui->actionRapportSecurite, &QAction::triggered, this, &MainWindow::onSecurityReport);
    qDebug() << "[MainWindow] Connexions établies";
}

void MainWindow::setupSecurityMonitoring()
{
    QTimer *uptimeTimer = new QTimer(this);
    connect(uptimeTimer, &QTimer::timeout, this, &MainWindow::updateSessionInfo);
    uptimeTimer->start(1000);
    m_securityStats.failedAuthAttempts = 0;
    m_securityStats.malformedPackets = 0;
    m_securityStats.replayAttempts = 0;
    m_securityStats.successfulCommands = 0;
}

void MainWindow::setupSecurityStatsTable()
{
    if (!ui->securityStatsTable) {
        return;
    }
    ui->securityStatsTable->setRowCount(5);
    ui->securityStatsTable->setColumnCount(3);
    QStringList headers;
    headers << "Métrique" << "Valeur" << "État";
    ui->securityStatsTable->setHorizontalHeaderLabels(headers);
    QStringList metrics;
    metrics << "🔐 Tentatives d'auth échouées"
            << "📦 Paquets malformés"
            << "🔄 Tentatives de replay"
            << "✅ Commandes réussies"
            << "⏱️ Durée de session";
    for (int i = 0; i < metrics.size(); i++) {
        QTableWidgetItem *metricItem = new QTableWidgetItem(metrics[i]);
        QTableWidgetItem *valueItem = new QTableWidgetItem("0");
        QTableWidgetItem *statusItem = new QTableWidgetItem("✅ OK");
        metricItem->setFlags(metricItem->flags() & ~Qt::ItemIsEditable);
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
        ui->securityStatsTable->setItem(i, 0, metricItem);
        ui->securityStatsTable->setItem(i, 1, valueItem);
        ui->securityStatsTable->setItem(i, 2, statusItem);
    }
    ui->securityStatsTable->resizeColumnsToContents();
}

void MainWindow::loadSerialPorts()
{
    ui->portComboBox->clear();
    if (ui->portComboBox_config) {
        ui->portComboBox_config->clear();
    }
    const auto ports = QSerialPortInfo::availablePorts();
    int validPortCount = 0;
    for (const QSerialPortInfo &info : ports) {
        QString portName = info.portName();
        if (portName.startsWith("ttyS")) {
            continue;
        }
        QString displayText = portName;
        if (!info.description().isEmpty()) {
            displayText += " - " + info.description();
        }
        ui->portComboBox->addItem(displayText, portName);
        if (ui->portComboBox_config) {
            ui->portComboBox_config->addItem(displayText, portName);
        }
        validPortCount++;
    }
    if (validPortCount == 0) {
        ui->portComboBox->addItem("⚠️ Aucun port USB détecté");
        ui->connectButton->setEnabled(false);
        ui->statusbar->showMessage("Branchez un périphérique USB", 0);
        logSecurityEvent("⚠️ ATTENTION: Aucun port série disponible");
        qDebug() << "[MainWindow] Aucun port disponible";
    } else {
        ui->connectButton->setEnabled(true);
        ui->statusbar->showMessage(QString("%1 port(s) disponible(s)").arg(validPortCount), 2000);
        qDebug() << "[MainWindow]" << validPortCount << "port(s) détecté(s)";
    }
}

void MainWindow::onRefreshPorts()
{
    loadSerialPorts();
    ui->receiveTextEdit->append(
        QString("<span style='color: #00d9ff;'>[%1] 🔄 Liste des ports mise à jour</span>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
    );
    logSecurityEvent("🔄 Rafraîchissement de la liste des ports");
}

void MainWindow::onConnectClicked()
{
    if (m_controller->isConnected()) {
        m_controller->disconnectFromDevice();
        logSecurityEvent("🔌 Déconnexion du périphérique");
    } else {
        QString portName = ui->portComboBox->currentData().toString();
        if (portName.isEmpty() || portName.startsWith("⚠️")) {
            showStyledMessageBox(
                "Port invalide",
                "⚠️ Veuillez sélectionner un port série valide.\n\n"
                "Utilisez le bouton 'Rafraîchir' pour mettre à jour la liste.",
                QMessageBox::Warning
            );
            return;
        }
        int baudRate = ui->baudRateComboBox->currentText().toInt();
        m_controller->connectToDevice(portName, baudRate);
        ui->receiveTextEdit->append(
            QString("<span style='color: #10b981; font-weight: bold;'>"
                    "[%1] 🔌 Connexion à %2 @ %3 bauds...</span>")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
            .arg(portName)
            .arg(baudRate)
        );
        logSecurityEvent(QString("🔌 Tentative de connexion à %1 @ %2 bauds").arg(portName).arg(baudRate));
    }
}

void MainWindow::onAuthenticateClicked()
{
    if (m_isAuthenticated) {
        showStyledQuestion(
            "Déconnexion",
            "🔒 Voulez-vous terminer votre session sécurisée ?\n\n"
            "Vous devrez vous reconnecter pour utiliser les contrôles.",
            [this]() {
                m_isAuthenticated = false;
                updateAuthenticationState(false);
                ui->passwordLineEdit->clear();
                logSecurityEvent("🔓 Déconnexion de la session utilisateur");
                showStyledMessageBox(
                    "Session terminée",
                    "✅ Vous avez été déconnecté avec succès.",
                    QMessageBox::Information
                );
            },
            nullptr
        );
        return;
    }
    QString username = ui->usernameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        showStyledMessageBox(
            "Authentification",
            "⚠️ Veuillez entrer un nom d'utilisateur et un mot de passe.",
            QMessageBox::Warning
        );
        return;
    }
    if (username == "admin" && password == "password") {
        m_isAuthenticated = true;
        m_sessionStartTime = QDateTime::currentDateTime();
        updateAuthenticationState(true);
        ui->receiveTextEdit->append(
            QString("<span style='color: #10b981; font-weight: bold;'>"
                    "[%1] ✅ Authentification réussie - Session ouverte</span>")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        );
        logSecurityEvent(QString("✅ Authentification réussie pour l'utilisateur: %1").arg(username));
        m_controller->sendCustomCommand(QString("AUTH:%1:%2").arg(username).arg(password));
        ui->mainTabWidget->setCurrentIndex(0);
        showStyledMessageBox(
            "Authentification réussie",
            "✅ Bienvenue " + username + " !\n\n"
            "Vous pouvez maintenant utiliser tous les contrôles.",
            QMessageBox::Information
        );
    } else {
        m_securityStats.failedAuthAttempts++;
        updateSecurityStats();
        showStyledMessageBox(
            "Authentification échouée",
            "❌ Nom d'utilisateur ou mot de passe incorrect.\n\n"
            "Veuillez réessayer.",
            QMessageBox::Critical
        );
        logSecurityEvent(QString("❌ Échec d'authentification pour: %1").arg(username));
        ui->passwordLineEdit->clear();
        ui->passwordLineEdit->setFocus();
    }
}

void MainWindow::onSendClicked()
{
    if (!checkAuthentication()) {
        return;
    }
    QString text = ui->commandLineEdit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    m_controller->sendCustomCommand(text);
    ui->receiveTextEdit->append(
        QString("<span style='color: #00d9ff; font-weight: bold;'>"
                "[%1] 📤 TX: %2</span>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(text)
    );
    m_securityStats.successfulCommands++;
    updateSecurityStats();
    logSecurityEvent(QString("📤 Commande envoyée: %1").arg(text));
    ui->commandLineEdit->clear();
    ui->commandLineEdit->setFocus();
}

void MainWindow::onGetTempClicked()
{
    if (!checkAuthentication()) return;
    m_controller->requestTemperature();
    logSecurityEvent("🌡️ Requête: Température");
}

void MainWindow::onGetVoltageClicked()
{
    if (!checkAuthentication()) return;
    m_controller->requestVoltage();
    logSecurityEvent("⚡ Requête: Tension");
}

void MainWindow::onGetStatusClicked()
{
    if (!checkAuthentication()) return;
    m_controller->requestStatus();
    logSecurityEvent("📊 Requête: Status complet");
}

void MainWindow::onLedOnClicked()
{
    if (!checkAuthentication()) return;
    m_controller->setLed(true);
    logSecurityEvent("💡 Commande: LED ON");
}

void MainWindow::onLedOffClicked()
{
    if (!checkAuthentication()) return;
    m_controller->setLed(false);
    logSecurityEvent("🔴 Commande: LED OFF");
}

void MainWindow::onPwmSliderChanged(int value)
{
    ui->pwmValueLabel->setText(QString("Duty Cycle: %1%").arg(value));
    ui->pwmValueLCD->display(value);
}

void MainWindow::onPwmSliderReleased()
{
    if (!checkAuthentication()) return;
    int value = ui->pwmSlider->value();
    m_controller->setPwm(static_cast<uint8_t>(value));
    logSecurityEvent(QString("⚙️ Commande: PWM = %1%").arg(value));
}

void MainWindow::onResetClicked()
{
    if (!checkAuthentication()) return;
    showStyledQuestion(
        "Confirmation",
        "⚠️ Voulez-vous vraiment réinitialiser le STM32 ?\n\n"
        "Cette action va redémarrer le microcontrôleur.",
        [this]() {
            m_controller->resetDevice();
            logSecurityEvent("🔄 COMMANDE CRITIQUE: Reset du STM32");
            showStyledMessageBox(
                "Reset en cours",
                "✅ Commande de reset envoyée au STM32.\n\n"
                "Le microcontrôleur va redémarrer.",
                QMessageBox::Information
            );
        },
        nullptr
    );
}

void MainWindow::onDataReceived(const QString &data)
{
    QString text = data.trimmed();
    if (text.isEmpty()) return;
    QString color = "#10b981";
    QString icon = "📥";
    if (text.contains("ERROR") || text.contains("error")) {
        color = "#ef4444";
        icon = "❌";
        m_securityStats.malformedPackets++;
        updateSecurityStats();
    } else if (text.contains("OK") || text.contains("ok")) {
        color = "#00d9ff";
        icon = "✅";
    } else if (text.contains("TEMP") || text.contains("temp")) {
        color = "#f59e0b";
        icon = "🌡️";
    } else if (text.contains("VOLTAGE") || text.contains("voltage")) {
        color = "#f59e0b";
        icon = "⚡";
    } else if (text.contains("STATUS") || text.contains("status")) {
        color = "#8b5cf6";
        icon = "📊";
    } else if (text.contains("READY")) {
        color = "#10b981";
        icon = "🚀";
    }
    ui->receiveTextEdit->append(
        QString("<span style='color: %1;'>[%2] %3 <b>%4</b></span>")
        .arg(color)
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(icon)
        .arg(text.toHtmlEscaped())
    );
    QScrollBar *scrollBar = ui->receiveTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onConnectionChanged(bool connected)
{
    updateConnectionState(connected);
    if (connected) {
        ui->receiveTextEdit->append(
            QString("<span style='color: #10b981; font-weight: bold;'>"
                    "[%1] ✅ Connexion établie avec succès</span>")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        );
        logSecurityEvent("✅ Connexion établie avec le STM32");
    } else {
        ui->receiveTextEdit->append(
            QString("<span style='color: #f59e0b; font-weight: bold;'>"
                    "[%1] ⚠️ Déconnexion du port série</span><br>")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        );
        logSecurityEvent("⚠️ Déconnexion du STM32");
        if (m_isAuthenticated) {
            m_isAuthenticated = false;
            updateAuthenticationState(false);
        }
    }
}

void MainWindow::onTemperatureUpdated(float temperature)
{
    ui->receiveTextEdit->append(
        QString("<span style='color: #f59e0b;'>"
                "[%1] 🌡️ Température: <b>%.1f°C</b></span>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(temperature)
    );
}

void MainWindow::onVoltageUpdated(float voltage)
{
    ui->receiveTextEdit->append(
        QString("<span style='color: #f59e0b;'>"
                "[%1] ⚡ Tension: <b>%.2fV</b></span>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(voltage)
    );
}

void MainWindow::onStatusUpdated(const QJsonObject &status)
{
    QString statusText = QJsonDocument(status).toJson(QJsonDocument::Compact);
    ui->receiveTextEdit->append(
        QString("<span style='color: #8b5cf6;'>"
                "[%1] 📊 STATUS: <b>%2</b></span>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(statusText)
    );
}

void MainWindow::onDeviceError(const QString &error)
{
    ui->statusbar->showMessage("⚠️ Erreur: " + error, 5000);
    ui->receiveTextEdit->append(
        QString("<span style='color: #ef4444; font-weight: bold;'>"
                "[%1] ❌ ERREUR: %2</span>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(error.toHtmlEscaped())
    );
    logSecurityEvent(QString("❌ ERREUR: %1").arg(error));
    m_securityStats.malformedPackets++;
    updateSecurityStats();
    qDebug() << "[MainWindow] Erreur:" << error;
}

void MainWindow::onClearMonitor()
{
    ui->receiveTextEdit->clear();
    ui->receiveTextEdit->append(
        QString("<span style='color: #6b7280; font-style: italic;'>"
                "[%1] 🗑️ Moniteur effacé</span><br>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
    );
}

void MainWindow::onClearSecurityLogs()
{
    ui->securityLogsTextEdit->clear();
    ui->securityLogsTextEdit->append(
        QString("<span style='color: #6b7280; font-style: italic;'>"
                "[%1] 🗑️ Logs de sécurité effacés</span><br>")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
    );
}

void MainWindow::onExportSecurityLogs()
{
    QString defaultFileName = QDateTime::currentDateTime().toString("'security_logs_'yyyyMMdd'_'HHmmss'.txt'");
    QString fileName = QFileDialog::getSaveFileName(this, "Exporter les logs de sécurité", defaultFileName, "Fichiers texte (*.txt);;Tous les fichiers (*)");
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "================================================================\n";
        out << "       LOGS DE SÉCURITÉ - STM32 SecureLink\n";
        out << "================================================================\n";
        out << "Date: " << QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") << "\n";
        out << "================================================================\n\n";
        out << ui->securityLogsTextEdit->toPlainText();
        file.close();
        ui->statusbar->showMessage("💾 Logs de sécurité exportés: " + fileName, 5000);
        logSecurityEvent(QString("💾 Export des logs: %1").arg(fileName));
        showStyledMessageBox(
            "Export réussi",
            "✅ Les logs de sécurité ont été exportés avec succès.\n\n"
            "Fichier: " + fileName,
            QMessageBox::Information
        );
        qDebug() << "[MainWindow] Logs de sécurité exportés:" << fileName;
    } else {
        showStyledMessageBox(
            "Erreur d'export",
            "❌ Impossible d'exporter le fichier:\n\n" + file.errorString(),
            QMessageBox::Critical
        );
    }
}

void MainWindow::onExportLogs()
{
    QString defaultFileName = QDateTime::currentDateTime().toString("'logs_'yyyyMMdd'_'HHmmss'.txt'");
    QString fileName = QFileDialog::getSaveFileName(this, "Sauvegarder les logs", defaultFileName, "Fichiers texte (*.txt);;Tous les fichiers (*)");
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "================================================================\n";
        out << "       LOGS INTERFACE STM32 SecureLink\n";
        out << "       Architecture MVC Sécurisée\n";
        out << "================================================================\n";
        out << "Date: " << QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss") << "\n";
        out << "================================================================\n\n";
        out << ui->receiveTextEdit->toPlainText();
        file.close();
        ui->statusbar->showMessage("💾 Logs sauvegardés: " + fileName, 5000);
        logSecurityEvent(QString("💾 Export des logs: %1").arg(fileName));
        showStyledMessageBox(
            "Export réussi",
            "✅ Les logs ont été sauvegardés avec succès.\n\n"
            "Fichier: " + fileName,
            QMessageBox::Information
        );
        qDebug() << "[MainWindow] Logs sauvegardés:" << fileName;
    } else {
        showStyledMessageBox(
            "Erreur d'export",
            "❌ Impossible de sauvegarder le fichier:\n\n" + file.errorString(),
            QMessageBox::Critical
        );
    }
}

void MainWindow::onSecurityReport()
{
    QString report = QString(
        "╔═══════════════════════════════════════════════════════════════╗\n"
        "║           RAPPORT DE SÉCURITÉ - STM32 SecureLink             ║\n"
        "╠═══════════════════════════════════════════════════════════════╣\n"
        "║  Date: %1                                  ║\n"
        "╠═══════════════════════════════════════════════════════════════╣\n"
        "║  🔐 Tentatives d'auth échouées: %2                            ║\n"
        "║  📦 Paquets malformés: %3                                     ║\n"
        "║  🔄 Tentatives de replay: %4                                  ║\n"
        "║  ✅ Commandes réussies: %5                                    ║\n"
        "║  ⏱️  Durée de session: %6                                     ║\n"
        "╠═══════════════════════════════════════════════════════════════╣\n"
        "║  État: %7                                                     ║\n"
        "╚═══════════════════════════════════════════════════════════════╝"
    ).arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss"))
     .arg(m_securityStats.failedAuthAttempts)
     .arg(m_securityStats.malformedPackets)
     .arg(m_securityStats.replayAttempts)
     .arg(m_securityStats.successfulCommands)
     .arg(getSessionDuration())
     .arg(getSecurityLevel() >= 80 ? "🟢 SÉCURISÉ" : "🟡 ATTENTION REQUISE");
    showStyledMessageBox("Rapport de Sécurité", report, QMessageBox::Information);
    logSecurityEvent("📊 Génération du rapport de sécurité");
}

void MainWindow::updateConnectionState(bool connected)
{
    if (connected) {
        ui->statusbar->showMessage("🟢 Connecté - Communication active", 0);
        ui->connectButton->setText("Déconnecter");
        ui->connIndicator->setStyleSheet("background-color: #10b981; border-radius: 10px; border: 2px solid #059669;");
        ui->sendButton->setEnabled(m_isAuthenticated);
        ui->getTempButton->setEnabled(m_isAuthenticated);
        ui->getVoltageButton->setEnabled(m_isAuthenticated);
        ui->getStatusButton->setEnabled(m_isAuthenticated);
        ui->ledOnButton->setEnabled(m_isAuthenticated);
        ui->ledOffButton->setEnabled(m_isAuthenticated);
        ui->pwmSlider->setEnabled(m_isAuthenticated);
        ui->resetButton->setEnabled(m_isAuthenticated);
        ui->commandLineEdit->setEnabled(m_isAuthenticated);
        ui->authenticateButton->setEnabled(true);
        ui->portComboBox->setEnabled(false);
        ui->baudRateComboBox->setEnabled(false);
        ui->refreshPortsButton->setEnabled(false);
    } else {
        ui->statusbar->showMessage("⚫ Déconnecté", 0);
        ui->connectButton->setText("Connecter");
        ui->connIndicator->setStyleSheet("background-color: #ef4444; border-radius: 10px; border: 2px solid #dc2626;");
        ui->sendButton->setEnabled(false);
        ui->getTempButton->setEnabled(false);
        ui->getVoltageButton->setEnabled(false);
        ui->getStatusButton->setEnabled(false);
        ui->ledOnButton->setEnabled(false);
        ui->ledOffButton->setEnabled(false);
        ui->pwmSlider->setEnabled(false);
        ui->resetButton->setEnabled(false);
        ui->commandLineEdit->setEnabled(false);
        ui->authenticateButton->setEnabled(false);
        ui->portComboBox->setEnabled(true);
        ui->baudRateComboBox->setEnabled(true);
        ui->refreshPortsButton->setEnabled(true);
    }
}

void MainWindow::updateAuthenticationState(bool authenticated)
{
    m_isAuthenticated = authenticated;
    if (authenticated) {
        ui->authStatusLabel->setText("AUTHENTIFIE");
        if (ui->authStatusFrame) {
            ui->authStatusFrame->setStyleSheet(
                "QFrame {"
                "    background-color: #0a0e27;"
                "    border: 2px solid #10b981;"
                "    border-radius: 8px;"
                "    padding: 15px;"
                "}"
            );
        }
        ui->authStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #10b981; border: none; background: transparent;");
        if (ui->authSubtitleLabel) {
            ui->authSubtitleLabel->setText("Session active - Tous les controles disponibles");
        }
        ui->authIndicator->setStyleSheet("background-color: #10b981; border-radius: 10px; border: 2px solid #059669;");
        ui->encryptIndicator->setStyleSheet("background-color: #10b981; border-radius: 10px; border: 2px solid #059669;");
        if (m_controller->isConnected()) {
            ui->sendButton->setEnabled(true);
            ui->getTempButton->setEnabled(true);
            ui->getVoltageButton->setEnabled(true);
            ui->getStatusButton->setEnabled(true);
            ui->ledOnButton->setEnabled(true);
            ui->ledOffButton->setEnabled(true);
            ui->pwmSlider->setEnabled(true);
            ui->resetButton->setEnabled(true);
            ui->commandLineEdit->setEnabled(true);
        }
        ui->usernameLineEdit->setEnabled(false);
        ui->passwordLineEdit->setEnabled(false);
        ui->authenticateButton->setText("SE DECONNECTER");
    } else {
        ui->authStatusLabel->setText("NON AUTHENTIFIE");
        if (ui->authStatusFrame) {
            ui->authStatusFrame->setStyleSheet(
                "QFrame {"
                "    background-color: #0a0e27;"
                "    border: 2px solid #ef4444;"
                "    border-radius: 8px;"
                "    padding: 15px;"
                "}"
            );
        }
        ui->authStatusLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #ef4444; border: none; background: transparent;");
        if (ui->authSubtitleLabel) {
            ui->authSubtitleLabel->setText("Connectez-vous pour acceder aux controles");
        }
        ui->authIndicator->setStyleSheet("background-color: #6b7280; border-radius: 10px; border: 2px solid #4b5563;");
        ui->encryptIndicator->setStyleSheet("background-color: #6b7280; border-radius: 10px; border: 2px solid #4b5563;");
        ui->sendButton->setEnabled(false);
        ui->getTempButton->setEnabled(false);
        ui->getVoltageButton->setEnabled(false);
        ui->getStatusButton->setEnabled(false);
        ui->ledOnButton->setEnabled(false);
        ui->ledOffButton->setEnabled(false);
        ui->pwmSlider->setEnabled(false);
        ui->resetButton->setEnabled(false);
        ui->commandLineEdit->setEnabled(false);
        ui->usernameLineEdit->setEnabled(true);
        ui->passwordLineEdit->setEnabled(true);
        ui->authenticateButton->setText("SE CONNECTER");
    }
    updateSecurityLevel();
}

void MainWindow::updateSecurityStats()
{
    if (ui->securityStatsTable && ui->securityStatsTable->rowCount() >= 5) {
        ui->securityStatsTable->item(0, 1)->setText(QString::number(m_securityStats.failedAuthAttempts));
        ui->securityStatsTable->item(1, 1)->setText(QString::number(m_securityStats.malformedPackets));
        ui->securityStatsTable->item(2, 1)->setText(QString::number(m_securityStats.replayAttempts));
        ui->securityStatsTable->item(3, 1)->setText(QString::number(m_securityStats.successfulCommands));
        ui->securityStatsTable->item(4, 1)->setText(getSessionDuration());
        ui->securityStatsTable->item(0, 2)->setText(m_securityStats.failedAuthAttempts == 0 ? "✅ OK" : "⚠️ ATTENTION");
        ui->securityStatsTable->item(1, 2)->setText(m_securityStats.malformedPackets < 10 ? "✅ OK" : "⚠️ ATTENTION");
        ui->securityStatsTable->item(2, 2)->setText(m_securityStats.replayAttempts == 0 ? "✅ OK" : "⚠️ ATTENTION");
    }
    if (ui->failedAuthCountLabel) {
        ui->failedAuthCountLabel->setText(QString::number(m_securityStats.failedAuthAttempts));
    }
    if (ui->malformedPacketsCountLabel) {
        ui->malformedPacketsCountLabel->setText(QString::number(m_securityStats.malformedPackets));
    }
    if (ui->successfulCommandsCountLabel) {
        ui->successfulCommandsCountLabel->setText(QString::number(m_securityStats.successfulCommands));
    }
    updateSecurityLevel();
}

void MainWindow::updateSecurityLevel()
{
    int level = getSecurityLevel();
    ui->securityLevelBar->setValue(level);
    QString status;
    QString chunkStyle;
    if (level >= 80) {
        status = "SECURISE";
        chunkStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #10b981, stop:1 #059669);";
        ui->threatIndicator->setStyleSheet("background-color: #10b981; border-radius: 10px; border: 2px solid #059669;");
    } else if (level >= 50) {
        status = "ATTENTION";
        chunkStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #f59e0b, stop:1 #d97706);";
        ui->threatIndicator->setStyleSheet("background-color: #f59e0b; border-radius: 10px; border: 2px solid #d97706;");
    } else {
        status = "CRITIQUE";
        chunkStyle = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ef4444, stop:1 #dc2626);";
        ui->threatIndicator->setStyleSheet("background-color: #ef4444; border-radius: 10px; border: 2px solid #dc2626;");
    }
    ui->securityLevelBar->setFormat("%p% - " + status);
    ui->securityLevelBar->setStyleSheet(
        "QProgressBar {"
        "    border: none;"
        "    border-radius: 8px;"
        "    text-align: center;"
        "    background-color: #16213e;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "    font-size: 11pt;"
        "}"
        "QProgressBar::chunk {"
        "    " + chunkStyle +
        "    border-radius: 8px;"
        "}"
    );
}

int MainWindow::getSecurityLevel()
{
    int level = 100;
    level -= m_securityStats.failedAuthAttempts * 10;
    level -= m_securityStats.malformedPackets * 2;
    level -= m_securityStats.replayAttempts * 15;
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return level;
}

void MainWindow::updateSessionInfo()
{
    QString sessionInfo = QString("Session: %1 | Uptime: %2")
        .arg(m_isAuthenticated ? getSessionDuration() : "--")
        .arg(m_controller->isConnected() ? getUptime() : "N/A");
    ui->sessionTimeLabel->setText(sessionInfo);
    if (ui->securityStatsTable && ui->securityStatsTable->rowCount() > 4) {
        ui->securityStatsTable->item(4, 1)->setText(getSessionDuration());
    }
}

QString MainWindow::getSessionDuration()
{
    if (!m_isAuthenticated) {
        return "--";
    }
    qint64 seconds = m_sessionStartTime.secsTo(QDateTime::currentDateTime());
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    return QString("%1h %2m %3s").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

QString MainWindow::getUptime()
{
    return "N/A";
}

void MainWindow::logSecurityEvent(const QString &event)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp).arg(event);
    ui->securityLogsTextEdit->append(QString("<span style='color: #00d9ff;'>%1</span>").arg(logEntry.toHtmlEscaped()));
    if (ui->autoScrollLogsCheckBox && ui->autoScrollLogsCheckBox->isChecked()) {
        QScrollBar *scrollBar = ui->securityLogsTextEdit->verticalScrollBar();
        scrollBar->setValue(scrollBar->maximum());
    }
}

bool MainWindow::checkAuthentication()
{
    if (!m_isAuthenticated) {
        showStyledMessageBox(
            "Non authentifié",
            "⚠️ Vous devez vous authentifier avant d'effectuer cette action.\n\n"
            "Allez dans l'onglet 'Sécurité' pour vous connecter.",
            QMessageBox::Warning
        );
        ui->mainTabWidget->setCurrentIndex(1);
        return false;
    }
    return true;
}
