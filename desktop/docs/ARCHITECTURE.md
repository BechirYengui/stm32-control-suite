# Architecture du Projet - STM32 Interface

## Table des matières

1. [Vue d'ensemble](#vue-densemble)
2. [Architecture MVC](#architecture-mvc)
3. [Couche Communication](#couche-communication)
4. [Threading et Asynchronisme](#threading-et-asynchronisme)
5. [Protocole JSON](#protocole-json)
6. [Firmware STM32 avec DMA](#firmware-stm32-avec-dma)
7. [Flux de données](#flux-de-données)
8. [Diagrammes](#diagrammes)

---

## Vue d'ensemble

Le projet implémente une **architecture MVC modulaire** avec une **couche de communication robuste** basée sur:
- **QThread** pour l'asynchronisme
- **DMA** pour des transferts efficaces côté STM32
- **Protocole JSON** pour des échanges structurés
- **Interruptions** pour la réactivité

### Principes de conception

1. **Séparation des responsabilités** (MVC)
2. **Communication non-bloquante** (Threading)
3. **Sérialisation structurée** (JSON)
4. **Efficacité** (DMA, interruptions)
5. **Extensibilité** (interfaces claires)
6. **Maintenabilité** (code modulaire)

---

## Architecture MVC

### Schéma général

```
┌─────────────────────────────────────────────────────────────┐
│                         Application                          │
│                        (QApplication)                        │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
    ┌───────┐    ┌──────────┐    ┌───────┐
    │ MODEL │    │CONTROLLER│    │ VIEW  │
    └───┬───┘    └────┬─────┘    └───┬───┘
        │             │              │
        │             │              │
    ┌───▼─────────────▼──────────────▼───┐
    │      COMMUNICATION LAYER            │
    │     (SerialManager + Worker)        │
    └────────────────┬────────────────────┘
                     │
                     ▼
              ┌─────────────┐
              │  STM32 + DMA│
              └─────────────┘
```

### 1. MODEL (Modèle de données)

#### `DeviceState.h/cpp`
**Responsabilité**: Représenter l'état complet du dispositif STM32

```cpp
class DeviceState : public QObject {
    Q_OBJECT
    Q_PROPERTY(float temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    // ... autres propriétés
    
private:
    float m_temperature;
    float m_voltage;
    uint8_t m_pwmDutyCycle;
    bool m_ledState;
    // ...
};
```

**Fonctionnalités**:
- Stocke toutes les variables d'état
- Émet des signaux lors de changements
- Sérialisation JSON
- Compatible avec QML (Q_PROPERTY)

#### `DataModel.h/cpp`
**Responsabilité**: Gérer l'historique des mesures

```cpp
class DataModel : public QObject {
    Q_OBJECT
    
public:
    struct DataPoint {
        QDateTime timestamp;
        double value;
    };
    
    void addTemperaturePoint(float temperature);
    QVector<DataPoint> getTemperatureHistory() const;
    double getTemperatureAverage() const;
    
private:
    QVector<DataPoint> m_temperatureHistory;
    QMutex m_mutex;  // Thread-safe
};
```

**Fonctionnalités**:
- Historique temporel des données
- Statistiques (moyenne, min, max)
- Thread-safe (QMutex)
- Limité à N points configurables

---

### 2. VIEW (Interface utilisateur)

#### `MainWindow.h/cpp/ui`
**Responsabilité**: Interface graphique principale

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(DeviceController *controller, QWidget *parent = nullptr);
    
private slots:
    void onConnectClicked();
    void onTemperatureUpdated(float temp);
    void onLedControlClicked();
    
private:
    Ui::MainWindow *ui;
    DeviceController *m_controller;
};
```

**Fonctionnalités**:
- Interface Qt Widgets
- Boutons de commandes rapides
- Moniteur série avec coloration syntaxique
- Affichage de l'état en temps réel

#### `ChartWidget.h/cpp` (optionnel)
**Responsabilité**: Graphiques temps réel

```cpp
class ChartWidget : public QWidget {
    Q_OBJECT
    
public:
    void addTemperaturePoint(float temp);
    void addVoltagePoint(float voltage);
    
private:
    QChart *m_chart;
    QLineSeries *m_temperatureSeries;
    QLineSeries *m_voltageSeries;
};
```

---

### 3. CONTROLLER (Logique métier)

#### `DeviceController.h/cpp`
**Responsabilité**: Coordonner modèle et communication

```cpp
class DeviceController : public QObject {
    Q_OBJECT
    
public:
    // Commandes de contrôle
    void setLed(bool state);
    void setPwm(uint8_t dutyCycle);
    void requestTemperature();
    
    // Accès aux modèles
    DeviceState* deviceState() const;
    DataModel* dataModel() const;
    
private slots:
    void handleDataReceived(const QByteArray &data);
    void parseJsonResponse(const QJsonObject &json);
    
private:
    DeviceState *m_deviceState;
    DataModel *m_dataModel;
    SerialManager *m_serialManager;
};
```

**Responsabilités**:
1. Envoyer des commandes au STM32
2. Parser les réponses JSON/texte
3. Mettre à jour les modèles
4. Émettre des signaux vers la vue
5. Implémenter la logique métier

**Flux typique**:
```
User clicks "LED ON" button
    ↓
MainWindow::onLedOnClicked()
    ↓
controller->setLed(true)
    ↓
JsonProtocol::encodeSetLed(true)
    ↓
serialManager->sendCommand(json)
    ↓
SerialWorker::sendData() [dans son thread]
    ↓
UART TX avec DMA
    ↓
STM32 reçoit et traite
    ↓
STM32 répond via UART RX DMA
    ↓
SerialWorker::handleReadyRead()
    ↓
emit dataReceived(json)
    ↓
controller->handleDataReceived()
    ↓
controller->parseJsonResponse()
    ↓
deviceState->setLedState(true)
    ↓
emit ledStateChanged(true)
    ↓
MainWindow updates UI
```

---

## Couche Communication

### Architecture de la communication

```
┌───────────────────────────────────────────────────────────┐
│                    Thread Principal                        │
│                                                            │
│  ┌──────────────────┐          ┌──────────────────┐      │
│  │  SerialManager   │◄────────►│ DeviceController │      │
│  │                  │  Signals │                  │      │
│  └────────┬─────────┘          └──────────────────┘      │
│           │                                                │
│           │ Queued Connection                              │
│           │                                                │
└───────────┼────────────────────────────────────────────────┘
            │
            │ Thread boundary
            ▼
┌───────────────────────────────────────────────────────────┐
│                   Thread Dédié (QThread)                   │
│                                                            │
│  ┌──────────────────┐                                     │
│  │   SerialWorker   │                                     │
│  │                  │                                     │
│  │  - RX/TX queue   │                                     │
│  │  - Buffer mgmt   │                                     │
│  │  - Error handling│                                     │
│  └────────┬─────────┘                                     │
│           │                                                │
│           ▼                                                │
│  ┌──────────────────┐                                     │
│  │   QSerialPort    │                                     │
│  └────────┬─────────┘                                     │
└───────────┼────────────────────────────────────────────────┘
            │
            ▼
         [UART]
```

### `SerialManager.h/cpp`

**Rôle**: Interface de haut niveau, gère le thread worker

```cpp
class SerialManager : public QObject {
    Q_OBJECT
    
public:
    bool openPort(const QString &portName, qint32 baudRate);
    void closePort();
    bool sendCommand(const QByteArray &command);
    
signals:
    // Vers le thread principal
    void dataReceived(const QByteArray &data);
    void connectionStatusChanged(bool connected);
    
    // Vers le worker (Queued)
    void requestOpenPort(const QString &portName, qint32 baudRate);
    void requestSendData(const QByteArray &data);
    
private:
    SerialWorker *m_worker;
    QThread *m_workerThread;
};
```

### `SerialWorker.h/cpp`

**Rôle**: Opérations I/O dans thread dédié

```cpp
class SerialWorker : public QObject {
    Q_OBJECT
    
public slots:
    void openPort(const QString &portName, qint32 baudRate);
    void sendData(const QByteArray &data);
    
signals:
    void portOpened(const QString &portName, qint32 baudRate);
    void dataReceived(const QByteArray &data);
    
private slots:
    void handleReadyRead();
    void processSendQueue();
    
private:
    QSerialPort *m_serialPort;
    QQueue<QByteArray> m_sendQueue;
    QMutex m_queueMutex;
};
```

**Avantages du threading**:
- ✅ UI jamais bloquée
- ✅ Opérations I/O asynchrones
- ✅ Queue pour gérer les envois multiples
- ✅ Thread-safe avec QMutex

---

## Threading et Asynchronisme

### Diagramme de séquence

```
Thread Principal          Worker Thread            STM32
     │                         │                     │
     │ requestOpenPort()       │                     │
     ├────────────────────────>│                     │
     │                         │ QSerialPort::open() │
     │                         ├────────────────────>│
     │                         │<────────────────────┤
     │ portOpened() signal     │                     │
     │<────────────────────────┤                     │
     │                         │                     │
     │ requestSendData()       │                     │
     ├────────────────────────>│                     │
     │                         │ enqueue data        │
     │                         │ UART TX             │
     │                         ├────────────────────>│
     │                         │                     │ Process
     │                         │                     │ command
     │                         │       UART RX       │
     │                         │<────────────────────┤
     │ dataReceived() signal   │                     │
     │<────────────────────────┤                     │
     │                         │                     │
```

### Synchronisation

**QMutex pour accès concurrent**:
```cpp
void SerialWorker::sendData(const QByteArray &data) {
    QMutexLocker locker(&m_queueMutex);
    m_sendQueue.enqueue(data);
    processSendQueue();
}
```

**Queued Connections**:
```cpp
connect(this, &SerialManager::requestSendData,
        m_worker, &SerialWorker::sendData,
        Qt::QueuedConnection);  // Cross-thread safe
```

---

## Protocole JSON

### Format des messages

#### Commande (PC → STM32)
```json
{
  "type": "cmd",
  "command": "SET_PWM",
  "params": {
    "duty": 75
  },
  "timestamp": "2025-01-15T10:30:00Z"
}
```

#### Réponse (STM32 → PC)
```json
{
  "type": "response",
  "data": {
    "pwm": 75,
    "status": "ok"
  },
  "timestamp": "2025-01-15T10:30:00Z"
}
```

#### Erreur
```json
{
  "type": "error",
  "message": "Invalid PWM value",
  "code": 400
}
```

#### Heartbeat
```json
{
  "type": "heartbeat",
  "data": {
    "rx_chars": 1234,
    "temp": 25.5,
    "pwm": 50
  }
}
```

### `JsonProtocol.h/cpp`

```cpp
class JsonProtocol : public QObject {
    Q_OBJECT
    
public:
    // Encodage
    static QByteArray encodeSetLed(bool state);
    static QByteArray encodeSetPwm(uint8_t duty);
    
    // Décodage
    static MessageType getMessageType(const QByteArray &data);
    static bool extractTemperature(const QJsonObject &json, float *temp);
    
    // Validation
    static bool isValidJson(const QByteArray &data);
};
```

---

## Firmware STM32 avec DMA

### Architecture DMA

```
┌─────────────────────────────────────────────────────────┐
│                      STM32F1xx                           │
│                                                          │
│  ┌──────────────┐                                       │
│  │     CPU      │                                       │
│  └──────┬───────┘                                       │
│         │                                                │
│         │ Configure                                      │
│         ▼                                                │
│  ┌──────────────┐        ┌──────────────┐              │
│  │  DMA1_CH6    │◄───────┤  USART2 RX   │              │
│  │  (RX Buffer) │        └──────────────┘              │
│  └──────────────┘                                       │
│         │                                                │
│         │ Full Transfer Complete IRQ                     │
│         │                                                │
│  ┌──────▼───────┐        ┌──────────────┐              │
│  │  DMA1_CH7    │───────>│  USART2 TX   │              │
│  │  (TX Buffer) │        └──────────────┘              │
│  └──────────────┘                                       │
│                                                          │
│  ┌──────────────┐        ┌──────────────┐              │
│  │  DMA1_CH1    │◄───────┤    ADC1      │              │
│  │  (ADC Buffer)│        └──────────────┘              │
│  └──────────────┘                                       │
│         │                                                │
│         │ Circular mode, Half/Full Transfer IRQ         │
│         ▼                                                │
│  [Process samples]                                       │
└─────────────────────────────────────────────────────────┘
```

### Configuration DMA UART RX

```c
void USART2_UART_DMA_Init(void) {
    // DMA pour RX (circular mode)
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;  // Buffer circulaire
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_usart2_rx);
    
    __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);
    
    // Démarre la réception
    HAL_UART_Receive_DMA(&huart2, uart_rx_buffer, UART_RX_BUFFER_SIZE);
}
```

### Gestion des interruptions DMA

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // Callback appelé automatiquement par le DMA
    uint16_t current_pos = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    
    // Parse les données reçues
    while (rx_read_pos != current_pos) {
        uint8_t c = uart_rx_buffer[rx_read_pos];
        rx_read_pos = (rx_read_pos + 1) % UART_RX_BUFFER_SIZE;
        
        if (c == '\
') {
            processCommand(cmd_buffer);
        }
    }
}
```

### Avantages du DMA

1. **CPU libre**: Le CPU n'est pas bloqué pendant les transferts
2. **Efficacité**: Transferts directs mémoire ↔ périphérique
3. **Pas de perte de données**: Buffer circulaire
4. **Réactivité**: Interruptions sur transfert complet
5. **Performance**: Jusqu'à 115200 bauds sans perte

---

## Flux de données

### Scénario complet: Contrôle PWM

```
[1] USER: Bouge le slider PWM à 75%
         │
         ▼
[2] MainWindow::onPwmSliderChanged(75)
         │
         ▼
[3] controller->setPwm(75)
         │
         ▼
[4] JsonProtocol::encodeSetPwm(75)
         │ Returns: {"type":"cmd","command":"SET_PWM","params":{"duty":75}}
         ▼
[5] serialManager->sendCommand(json)
         │
         ▼
[6] emit requestSendData(json)  [Queued → Worker Thread]
         │
         ▼
[7] SerialWorker::sendData(json)
         │ m_sendQueue.enqueue(json)
         ▼
[8] SerialWorker::processSendQueue()
         │
         ▼
[9] QSerialPort::write(json)
         │
         ▼
[10] UART TX via DMA
         │
         ▼
[11] STM32 DMA1_CH7 transfert
         │
         ▼
[12] STM32 parseJson()
         │ Extrait: command="SET_PWM", params={"duty":75}
         ▼
[13] STM32 setPWM(75)
         │ __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 749)
         ▼
[14] STM32 sendJsonResponse()
         │ Crée: {"type":"response","data":{"pwm":75}}
         ▼
[15] UART RX via DMA
         │
         ▼
[16] STM32 DMA1_CH6 → Buffer circulaire
         │
         ▼
[17] HAL_UART_RxCpltCallback()
         │
         ▼
[18] SerialWorker::handleReadyRead()
         │
         ▼
[19] emit dataReceived(json)  [→ Main Thread]
         │
         ▼
[20] controller->handleDataReceived(json)
         │
         ▼
[21] controller->parseJsonResponse(json)
         │
         ▼
[22] deviceState->setPwmDutyCycle(75)
         │
         ▼
[23] emit pwmDutyCycleChanged(75)
         │
         ▼
[24] MainWindow::onPwmUpdated(75)
         │
         ▼
[25] UI Update: Label "PWM: 75%"
         │
         ▼
[26] DataModel::addPwmPoint(75, timestamp)
```

**Temps total estimé**: 50-100ms (selon baudrate)

---

## Diagrammes

### Diagramme de classes (simplifié)

```
┌──────────────────┐
│  QApplication    │
└────────┬─────────┘
         │
         │ has-a
         ▼
┌──────────────────┐       ┌──────────────────┐
│ DeviceController │──────>│   DeviceState    │
│                  │ owns  │                  │
│                  │       │  + temperature   │
│                  │       │  + voltage       │
│                  │       │  + pwmDutyCycle  │
└────────┬─────────┘       └──────────────────┘
         │
         │ owns
         ▼
┌──────────────────┐       ┌──────────────────┐
│  SerialManager   │──────>│  SerialWorker    │
│                  │ owns  │                  │
│                  │       │ runs in QThread  │
└──────────────────┘       └──────────────────┘
         │
         │ uses
         ▼
┌──────────────────┐
│  JsonProtocol    │
│  (static class)  │
└──────────────────┘
```

### Diagramme d'états

```
            [Disconnected]
                 │
                 │ openPort()
                 ▼
            [Connecting]
                 │
    ┌────────────┼────────────┐
    │ Success    │            │ Failure
    ▼            │            ▼
[Connected]      │       [Error]
    │            │            │
    │            │            │ retry
    │            └────────────┘
    │
    │ Commands exchanged
    │ (GET_TEMP, SET_PWM, etc.)
    │
    │ closePort() or disconnect
    ▼
[Disconnecting]
    │
    ▼
[Disconnected]
```

---

## Conclusion

Cette architecture offre:

✅ **Modularité**: Composants indépendants et réutilisables  
✅ **Performance**: DMA, threading, pas de blocage UI  
✅ **Robustesse**: Gestion d'erreurs, thread-safe  
✅ **Extensibilité**: Facile d'ajouter de nouvelles fonctionnalités  
✅ **Maintenabilité**: Code clair, bien organisé  
✅ **Professionnalisme**: Architecture industrielle standard  

---

**Document maintenu par**: Équipe de développement IMT Atlantique  
**Dernière mise à jour**: Janvier 2025
