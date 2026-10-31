# Guide de Migration - Intégration des Fichiers Originaux

## 📋 Vue d'Ensemble

Ce guide explique comment intégrer vos fichiers Qt originaux (`MainWindow.cpp`, `SerialManager.cpp`, etc.) dans la nouvelle architecture MVC améliorée.

---

## 🔄 Mapping des Fichiers

### Fichiers à Adapter

| Fichier Original | Nouveau Fichier | Action |
|------------------|----------------|--------|
| `MainWindow.h/cpp/ui` | `src/view/MainWindow.h/cpp/ui` | **Adapter** |
| `SerialManager.h/cpp` | `src/communication/SerialManager.h/cpp` | **Remplacer** |
| `main.cpp` | `src/main.cpp` | **Remplacer** |
| Firmware STM32 | `stm32_firmware/main_with_dma.c` | **Utiliser nouveau** |

### Nouveaux Fichiers (Architecture MVC)

| Fichier | Rôle | Création |
|---------|------|----------|
| `src/model/DeviceState.h/cpp` | État du dispositif | ✅ Créé |
| `src/model/DataModel.h/cpp` | Historique données | ✅ Créé |
| `src/controller/DeviceController.h/cpp` | Contrôleur MVC | ✅ Créé |
| `src/communication/SerialWorker.h/cpp` | Thread worker | ✅ Créé |
| `src/communication/JsonProtocol.h/cpp` | Protocole JSON | ✅ Créé |

---

## 🔧 Étape 1: Adapter MainWindow

### Modifications Nécessaires

#### 1.1 Header (`MainWindow.h`)

**Avant** (original):
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    SerialManager *m_serial;
};
```

**Après** (nouvelle architecture):
```cpp
#include "DeviceController.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(DeviceController *controller, QWidget *parent = nullptr);

private:
    DeviceController *m_controller;  // Au lieu de SerialManager
};
```

#### 1.2 Constructeur (`MainWindow.cpp`)

**Avant**:
```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new SerialManager(this))
{
    ui->setupUi(this);
    setupConnections();
}
```

**Après**:
```cpp
MainWindow::MainWindow(DeviceController *controller, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(controller)
{
    ui->setupUi(this);
    setupConnections();
}
```

#### 1.3 Connexions des Signaux

**Avant**:
```cpp
void MainWindow::setupConnections() {
    connect(m_serial, &SerialManager::dataReceived,
            this, &MainWindow::onDataReceived);
    connect(m_serial, &SerialManager::connectionStatusChanged,
            this, &MainWindow::onConnectionChanged);
}
```

**Après**:
```cpp
void MainWindow::setupConnections() {
    // Connexions au contrôleur
    connect(m_controller, &DeviceController::connectedChanged,
            this, &MainWindow::onConnectionChanged);
    
    connect(m_controller, &DeviceController::responseReceived,
            this, &MainWindow::onDataReceived);
    
    connect(m_controller, &DeviceController::temperatureUpdated,
            this, &MainWindow::onTemperatureUpdated);
    
    connect(m_controller, &DeviceController::voltageUpdated,
            this, &MainWindow::onVoltageUpdated);
    
    // Connexions aux boutons
    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    // ... autres boutons
}
```

#### 1.4 Slots de Commande

**Avant**:
```cpp
void MainWindow::onConnectClicked() {
    QString portName = ui->portComboBox->currentData().toString();
    int baudRate = ui->baudRateComboBox->currentText().toInt();
    m_serial->openPort(portName, baudRate);
}
```

**Après**:
```cpp
void MainWindow::onConnectClicked() {
    QString portName = ui->portComboBox->currentData().toString();
    int baudRate = ui->baudRateComboBox->currentText().toInt();
    m_controller->connectToDevice(portName, baudRate);
}
```

**Avant**:
```cpp
void MainWindow::onLedOnClicked() {
    m_serial->sendCommand("SET_LED=1\n");
}
```

**Après**:
```cpp
void MainWindow::onLedOnClicked() {
    m_controller->setLed(true);  // API haut niveau
}
```

---

## 📝 Étape 2: Modifications du main.cpp

**Avant** (original):
```cpp
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
```

**Après** (nouvelle architecture):
```cpp
#include <QApplication>
#include "MainWindow.h"
#include "DeviceController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setOrganizationName("IMT Atlantique");
    app.setApplicationName("STM32 Interface");
    
    // Création du contrôleur (MVC)
    DeviceController *controller = new DeviceController(&app);
    
    // Création de la vue
    MainWindow window(controller);
    window.show();
    
    return app.exec();
}
```

---

## 🔌 Étape 3: Utiliser les Nouveaux Fichiers

### 3.1 Remplacer SerialManager

**Ne modifiez PAS** votre ancien `SerialManager.h/cpp`. Utilisez les **nouveaux fichiers** qui incluent:
- Threading avec `SerialWorker`
- Support JSON avec `JsonProtocol`
- Gestion d'erreurs améliorée

### 3.2 Ajouter DeviceController

Le `DeviceController` est le **point central** de la nouvelle architecture:

```cpp
// Connexion
controller->connectToDevice("/dev/ttyACM0", 115200);

// Commandes (API haut niveau)
controller->setLed(true);
controller->setPwm(75);
controller->requestTemperature();
controller->requestStatus();

// Accès aux modèles
DeviceState *state = controller->deviceState();
qDebug() << "Temperature:" << state->temperature();

DataModel *dataModel = controller->dataModel();
auto history = dataModel->getTemperatureHistory();
```

---

## 🏗️ Étape 4: Compilation

### 4.1 Structure des Répertoires

```bash
stm32_interface_improved/
├── CMakeLists.txt
├── src/
│   ├── main.cpp                      # NOUVEAU
│   ├── model/                        # NOUVEAU
│   │   ├── DeviceState.{h,cpp}
│   │   └── DataModel.{h,cpp}
│   ├── view/                         # VOS FICHIERS ADAPTÉS
│   │   ├── MainWindow.h              # Modifier selon guide
│   │   ├── MainWindow.cpp            # Modifier selon guide
│   │   └── MainWindow.ui             # Garder tel quel
│   ├── controller/                   # NOUVEAU
│   │   └── DeviceController.{h,cpp}
│   └── communication/                # NOUVEAUX
│       ├── SerialManager.{h,cpp}     # Remplace ancien
│       ├── SerialWorker.{h,cpp}      # Nouveau
│       └── JsonProtocol.{h,cpp}      # Nouveau
└── stm32_firmware/
    └── main_with_dma.c               # NOUVEAU (DMA + JSON)
```

### 4.2 Commandes de Compilation

```bash
# 1. Copier vos fichiers originaux dans src/view/
cp /path/to/your/MainWindow.* src/view/

# 2. Adapter MainWindow selon le guide ci-dessus

# 3. Compiler
mkdir build && cd build
cmake ..
make -j$(nproc)

# 4. Tester
./STM32Interface
```

---

## 🚨 Points d'Attention

### Changements Importants

1. **SerialManager n'est plus directement accessible**
   - Passez par `DeviceController` au lieu de `SerialManager`
   - Exemple: `controller->requestTemperature()` au lieu de `serial->sendCommand("GET_TEMP\n")`

2. **Les commandes sont typées**
   - Plus de strings brutes
   - Utiliser les méthodes du contrôleur: `setLed(bool)`, `setPwm(uint8_t)`

3. **Les signaux ont changé**
   - `dataReceived(QByteArray)` → `responseReceived(QString)` + signaux spécifiques
   - `temperatureUpdated(float)`, `voltageUpdated(float)`, etc.

4. **Threading transparent**
   - Pas de changement dans votre code
   - La communication est maintenant asynchrone automatiquement

---

## 📊 Comparaison Avant/Après

### Envoi d'une Commande

**Avant**:
```cpp
// Approche bas niveau
m_serial->sendCommand("SET_PWM=75\n");
```

**Après**:
```cpp
// Approche haut niveau
m_controller->setPwm(75);
```

### Réception de Données

**Avant**:
```cpp
void MainWindow::onDataReceived(const QByteArray &data) {
    QString text = QString::fromUtf8(data);
    // Parse manuel du texte
    if (text.startsWith("TEMP:")) {
        // Extraction manuelle
    }
}
```

**Après**:
```cpp
void MainWindow::onTemperatureUpdated(float temperature) {
    // Donnée déjà parsée et typée
    ui->tempLabel->setText(QString("Temperature: %1°C").arg(temperature));
}
```

### Accès à l'État

**Avant**:
```cpp
// Pas de stockage d'état centralisé
// Chaque widget garde sa copie
```

**Après**:
```cpp
// État centralisé
DeviceState *state = m_controller->deviceState();
float currentTemp = state->temperature();
bool ledOn = state->ledState();
```

---

## ✅ Checklist de Migration

- [ ] Copier `MainWindow.{h,cpp,ui}` dans `src/view/`
- [ ] Modifier le constructeur de `MainWindow` pour accepter `DeviceController*`
- [ ] Remplacer `m_serial` par `m_controller` dans `MainWindow`
- [ ] Adapter les connexions de signaux
- [ ] Remplacer les appels `sendCommand()` par les méthodes du contrôleur
- [ ] Adapter les slots de réception de données
- [ ] Mettre à jour `main.cpp`
- [ ] Compiler et tester
- [ ] Flasher le nouveau firmware STM32 avec DMA
- [ ] Tester la communication PC ↔ STM32

---

## 🐛 Dépannage

### Erreur: "undefined reference to DeviceController"

**Solution**: Ajouter les fichiers du contrôleur au CMakeLists.txt:
```cmake
set(PROJECT_SOURCES
    # ...
    src/controller/DeviceController.h
    src/controller/DeviceController.cpp
)
```

### Erreur: "no matching function for call to MainWindow::MainWindow()"

**Solution**: Mettre à jour `main.cpp` pour passer le contrôleur:
```cpp
DeviceController *controller = new DeviceController(&app);
MainWindow window(controller);  // Passe le contrôleur
```

### Communication ne fonctionne pas

**Solutions**:
1. Vérifier que le firmware STM32 avec DMA est flashé
2. Vérifier les permissions du port série: `sudo usermod -a -G dialout $USER`
3. Vérifier le baudrate (115200)
4. Activer le mode debug: `qDebug() << "..."` dans les callbacks

---

## 📚 Ressources

- **ARCHITECTURE.md**: Documentation technique complète
- **README.md**: Guide d'utilisation
- **Code source**: Exemples dans chaque fichier .cpp

---

## 🎯 Résumé

| Aspect | Avant | Après |
|--------|-------|-------|
| **Architecture** | Monolithique | MVC modulaire |
| **Communication** | Bloquante | Asynchrone (QThread) |
| **Protocole** | Texte brut | JSON structuré |
| **STM32** | Interruptions | DMA + Interruptions |
| **API** | Bas niveau (strings) | Haut niveau (typée) |
| **État** | Dispersé | Centralisé (Model) |

---

**La migration apporte une architecture professionnelle, maintenable et performante, tout en conservant votre interface utilisateur existante !**
