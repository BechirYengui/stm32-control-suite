# Interface de Pilotage et Supervision STM32

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/votre-repo/stm32-interface)
[![Qt](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

**Projet développé **

## 📋 Description

Interface IHM professionnelle en C++/Qt pour le pilotage, le paramétrage et la supervision en temps réel d'un microcontrôleur STM32. Le projet implémente une architecture logicielle modulaire (MVC) avec une couche de communication robuste basée sur DMA, interruptions, et protocole JSON.

### 🎯 Fonctionnalités principales

- **Communication série asynchrone** avec threading (QThread) pour des performances optimales
- **Protocole JSON** pour échanges de données structurées
- **Architecture MVC** claire et maintenable
- **DMA + Interruptions** côté STM32 pour efficacité maximale
- **Interface graphique** double : Qt Widgets et QML
- **Graphiques temps réel** des mesures (température, tension, PWM)
- **Historique des données** avec statistiques
- **Configuration flexible** des paramètres de communication
- **Gestion d'erreurs robuste** avec feedback utilisateur

---

## 🏗️ Architecture du Projet

### Structure MVC (Model-View-Controller)

```
┌─────────────────────────────────────────────────────────────┐
│                         APPLICATION                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │     VIEW     │◄──►│  CONTROLLER  │◄──►│    MODEL     │  │
│  │              │    │              │    │              │  │
│  │ MainWindow   │    │   Device     │    │ DeviceState  │  │
│  │ ChartWidget  │    │  Controller  │    │  DataModel   │  │
│  │ QML Interface│    │              │    │              │  │
│  └──────────────┘    └───────┬──────┘    └──────────────┘  │
│                              │                               │
│                              ▼                               │
│                   ┌────────────────────┐                     │
│                   │   COMMUNICATION    │                     │
│                   │                    │                     │
│                   │  SerialManager     │                     │
│                   │  ├─ SerialWorker   │  ◄─── QThread      │
│                   │  ├─ JsonProtocol   │                     │
│                   │  └─ QSerialPort    │                     │
│                   └──────────┬─────────┘                     │
│                              │                               │
│                              ▼                               │
│                   ┌────────────────────┐                     │
│                   │   UART + DMA       │                     │
│                   │     (115200)       │                     │
│                   └──────────┬─────────┘                     │
└──────────────────────────────┼─────────────────────────────┘
                               │
                               ▼
              ┌─────────────────────────────────┐
              │         STM32F1xx               │
              │                                 │
              │  ├─ UART RX/TX avec DMA         │
              │  ├─ ADC avec DMA                │
              │  ├─ PWM (TIM2)                  │
              │  ├─ Protocole JSON              │
              │  └─ Gestion interruptions       │
              └─────────────────────────────────┘
```

### Composants principaux

#### 1. **Model (Modèle de données)**
- `DeviceState`: État complet du dispositif STM32
- `DataModel`: Historique des mesures avec statistiques

#### 2. **View (Interface utilisateur)**
- `MainWindow`: Interface Qt Widgets principale
- `ChartWidget`: Graphiques temps réel (optionnel)
- QML Interface: Interface moderne alternative

#### 3. **Controller (Logique métier)**
- `DeviceController`: Coordonne modèle et communication
- Parse les réponses JSON
- Gère la logique applicative

#### 4. **Communication Layer**
- `SerialManager`: Gestionnaire haut niveau
- `SerialWorker`: Worker thread pour I/O série
- `JsonProtocol`: Encodage/décodage JSON
- `QSerialPort`: Driver série Qt

---

## 🚀 Installation et Compilation

### Prérequis

- **Qt 5.15+** avec les modules suivants:
  - Qt Core
  - Qt Widgets
  - Qt SerialPort
  - Qt Qml / Quick (optionnel)
  - Qt Charts (optionnel)
  
- **CMake 3.16+**
- **Compilateur C++17** (GCC 7+, Clang 5+, MSVC 2017+)

### Linux (Ubuntu/Debian)

```bash
# Installation des dépendances
sudo apt update
sudo apt install qt5-default qtbase5-dev libqt5serialport5-dev
sudo apt install cmake build-essential git

# Modules optionnels
sudo apt install qtdeclarative5-dev qml-module-qtquick2
sudo apt install libqt5charts5-dev

# Clone du projet
git clone https://github.com/votre-repo/stm32-interface.git
cd stm32-interface

# Compilation
mkdir build && cd build
cmake ..
make -j$(nproc)

# Exécution
./STM32Interface
```

### Windows

```bash
# Avec Qt installé via Qt Online Installer
# Ouvrir Qt Creator → File → Open Project → CMakeLists.txt
# Ou en ligne de commande:

mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

# Exécution
STM32Interface.exe
```

### Options de compilation

```bash
# Build complet avec toutes les fonctionnalités
cmake -DBUILD_WITH_QML=ON -DBUILD_WITH_CHARTS=ON ..

# Build minimal (Widgets seulement)
cmake -DBUILD_WITH_QML=OFF -DBUILD_WITH_CHARTS=OFF ..

# Build avec tests
cmake -DBUILD_TESTS=ON ..
make && ctest

# Build avec documentation
cmake -DBUILD_DOCS=ON ..
make docs
```

---

## 📡 Configuration STM32

### Hardware

- **Microcontrôleur**: STM32F103 (ou compatible STM32F1xx)
- **Communication**: USART2 (PA2=TX, PA3=RX) @ 115200 bauds
- **ADC**: PA0 (entrée analogique 0-3.3V)
- **PWM**: PA1 (TIM2_CH2)
- **LED**: PC13 (active LOW)

### Firmware

Le firmware STM32 se trouve dans `stm32_firmware/main_with_dma.c`.

**Fonctionnalités du firmware:**
- ✅ UART avec DMA (RX et TX)
- ✅ ADC avec DMA en mode circulaire
- ✅ Interruptions optimisées
- ✅ Protocole JSON natif
- ✅ Parsing léger des commandes
- ✅ Gestion d'erreurs

**Compilation:**
```bash
# Utiliser STM32CubeIDE ou Makefile généré par CubeMX
# Flasher avec st-link:
st-flash write firmware.bin 0x8000000
```

---

## 💻 Utilisation

### Démarrage rapide

1. **Brancher le STM32** via USB (UART-USB converter)
2. **Lancer l'application** `./STM32Interface`
3. **Sélectionner le port série** (ex: `/dev/ttyACM0` ou `COM3`)
4. **Cliquer sur "Connecter"**
5. **Utiliser les commandes rapides** ou envoyer des commandes personnalisées

### Commandes disponibles

#### Mode Texte (compatibilité)
```
GET_TEMP         - Lecture température
GET_VOLTAGE      - Lecture tension ADC
GET_ADC_RAW      - Valeur ADC brute
STATUS           - État complet du système
SET_LED=0/1      - Contrôle LED
SET_PWM=0-100    - Réglage PWM (duty cycle en %)
TOGGLE_LED       - Inverse l'état de la LED
RESET            - Reset du microcontrôleur
```

#### Mode JSON (recommandé)

**Lecture température:**
```json
{
  "type": "cmd",
  "command": "GET_TEMP"
}
```

**Réponse:**
```json
{
  "type": "response",
  "data": {
    "temp": 25.5
  }
}
```

**Contrôle LED:**
```json
{
  "type": "cmd",
  "command": "SET_LED",
  "params": {
    "state": 1
  }
}
```

**Contrôle PWM:**
```json
{
  "type": "cmd",
  "command": "SET_PWM",
  "params": {
    "duty": 75
  }
}
```

**Status complet:**
```json
{
  "type": "response",
  "data": {
    "temp": 25.5,
    "voltage": 1.65,
    "adc": 2048,
    "pwm": 75,
    "led": 1,
    "uptime": 3600,
    "rx_chars": 1234
  }
}
```

---

## 🔧 API et Exemples de Code

### Utilisation du DeviceController

```cpp
#include "DeviceController.h"

// Création du contrôleur
DeviceController *controller = new DeviceController(this);

// Connexion au dispositif
controller->connectToDevice("/dev/ttyACM0", 115200);

// Envoi de commandes
controller->setLed(true);
controller->setPwm(50);
controller->requestTemperature();

// Réception des données
connect(controller, &DeviceController::temperatureUpdated,
        [](float temp) {
    qDebug() << "Temperature:" << temp << "°C";
});

// Accès au modèle de données
DeviceState *state = controller->deviceState();
qDebug() << "Current temp:" << state->temperature();

// Historique des données
DataModel *dataModel = controller->dataModel();
auto tempHistory = dataModel->getTemperatureHistory();
```

### Threading personnalisé

```cpp
// Le SerialWorker s'exécute dans son propre thread
// Les opérations I/O ne bloquent jamais l'UI

SerialManager *serial = new SerialManager(this);

// Envoi asynchrone
serial->sendCommand(data);  // Non-bloquant

// Réception dans le thread principal
connect(serial, &SerialManager::dataReceived,
        this, &MyClass::handleData);
```

---

## 📊 Technologies Utilisées

| Catégorie | Technologie | Usage |
|-----------|-------------|-------|
| **Langage** | C++17 | Application Qt |
| **Framework** | Qt 5.15+ | Interface graphique |
| **Architecture** | MVC | Organisation du code |
| **Threading** | QThread | Communication asynchrone |
| **Sérialisation** | JSON | Échange de données |
| **Communication** | QSerialPort | Driver série |
| **Build System** | CMake | Compilation |
| **Embedded** | C + HAL | Firmware STM32 |
| **DMA** | STM32 DMA | Transferts efficaces |
| **Version Control** | Git | Gestion de version |

---

## 🧪 Tests

```bash
# Compilation avec tests
cmake -DBUILD_TESTS=ON ..
make

# Exécution des tests
ctest --output-on-failure

# Tests unitaires individuels
./tests/test_devicestate
./tests/test_jsonprotocol
./tests/test_datamodel
```

---

## 📚 Documentation

### Génération de la documentation

```bash
cmake -DBUILD_DOCS=ON ..
make docs

# Documentation dans: build/docs/html/index.html
firefox build/docs/html/index.html
```

### Structure du code

```
stm32_interface_improved/
├── CMakeLists.txt           # Configuration CMake principale
├── README.md                # Ce fichier
├── LICENSE                  # Licence du projet
│
├── src/                     # Code source C++/Qt
│   ├── main.cpp             # Point d'entrée
│   ├── model/               # Modèles (MVC)
│   │   ├── DeviceState.{h,cpp}
│   │   └── DataModel.{h,cpp}
│   ├── view/                # Vues (MVC)
│   │   ├── MainWindow.{h,cpp,ui}
│   │   └── ChartWidget.{h,cpp}
│   ├── controller/          # Contrôleurs (MVC)
│   │   └── DeviceController.{h,cpp}
│   └── communication/       # Couche communication
│       ├── SerialManager.{h,cpp}
│       ├── SerialWorker.{h,cpp}
│       └── JsonProtocol.{h,cpp}
____ MainWindow.ui
```

---

## 🤝 Contribution

Les contributions sont les bienvenues ! Merci de suivre ces étapes:

1. Fork le projet
2. Créer une branche (`git checkout -b feature/AmazingFeature`)
3. Commit les changements (`git commit -m 'Add AmazingFeature'`)
4. Push vers la branche (`git push origin feature/AmazingFeature`)
5. Ouvrir une Pull Request

---

## 🐛 Dépannage

### Problème: Port série non détecté

```bash
# Linux: Vérifier les permissions
sudo usermod -a -G dialout $USER
# Se déconnecter/reconnecter

# Vérifier les ports disponibles
ls -l /dev/tty*
```

### Problème: Erreur de compilation Qt

```bash
# Vérifier l'installation de Qt
qmake --version

# Réinstaller Qt SerialPort
sudo apt install libqt5serialport5-dev
```

### Problème: DMA ne fonctionne pas (STM32)

- Vérifier la configuration des horloges DMA
- S'assurer que les interruptions DMA sont activées
- Vérifier les priorités d'interruption

---

## 📝 License

Ce projet est sous licence MIT. Voir le fichier [LICENSE](LICENSE) pour plus de détails.

---

## 👥 Auteurs

- **Bechir**

