# STM32 Desktop Interface

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
[![Qt](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-17-orange.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 📋 Description

C++/Qt desktop application for monitoring and controlling an STM32 microcontroller over a serial link. It follows an MVC architecture with a communication layer built on DMA, interrupts and a JSON wire protocol.

### Main features

- Asynchronous serial communication running on a dedicated `QThread`
- JSON protocol for structured commands and responses
- Clear MVC separation
- Two view options: Qt Widgets (default) and a minimal QML view
- In-memory history of measurements with basic statistics
- Configurable serial parameters
- Error feedback surfaced to the UI
- Unit tests (QtTest), Doxygen config, etc.

---

## 🏗️ Architecture

### MVC structure

```
┌─────────────────────────────────────────────────────────────┐
│                         APPLICATION                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │     VIEW     │◄──►│  CONTROLLER  │◄──►│    MODEL     │  │
│  │              │    │              │    │              │  │
│  │ MainWindow   │    │   Device     │    │ DeviceState  │  │
│  │ QML Interface│    │  Controller  │    │  DataModel   │  │
│  │              │    │              │    │              │  │
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
              │  ├─ UART RX/TX with DMA         │
              │  ├─ ADC with DMA                │
              │  ├─ PWM (TIM2)                  │
              │  ├─ JSON protocol               │
              │  └─ Interrupt handling          │
              └─────────────────────────────────┘
```

### Components

**Model.** `DeviceState` holds the current device snapshot (temperature, voltage, PWM duty, LED state). `DataModel` keeps a rolling history of measurements with basic stats.

**View.** `MainWindow` is the main Qt Widgets UI. An optional minimal QML view (`qml/main.qml`) reuses the same `DeviceState` exposed as a context property.

**Controller.** `DeviceController` glues the model and the communication layer together. It parses JSON responses and emits high-level signals consumed by the views.

**Communication.** `SerialManager` is the high-level facade; the actual I/O runs in `SerialWorker` on its own thread. `JsonProtocol` handles encoding and decoding. The underlying driver is Qt's `QSerialPort`.

---

## 🚀 Installation

### Requirements

- Qt 5.15+ with Core, Widgets and SerialPort (plus Qml/Quick if you want the QML view)
- CMake 3.16+
- A C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install qt5-default qtbase5-dev libqt5serialport5-dev
sudo apt install cmake build-essential git

# Optional: QML view
sudo apt install qtdeclarative5-dev qml-module-qtquick2 \
                 qml-module-qtquick-controls2 qml-module-qtquick-layouts

git clone https://github.com/BechirYengui/stm32-control-suite.git
cd stm32-control-suite/desktop

mkdir build && cd build
cmake ..
make -j$(nproc)

./STM32Interface
```

### Windows

```bash
# With Qt installed via the Qt Online Installer:
# Open Qt Creator → File → Open Project → CMakeLists.txt
# Or from the command line:

mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

STM32Interface.exe
```

### Build options

```bash
# Build with the QML view enabled
cmake -DBUILD_WITH_QML=ON ..

# Build with the unit tests
cmake -DBUILD_TESTS=ON ..
make && ctest
```

---

## STM32 setup

### Hardware

- MCU: STM32F103 (or any STM32F1xx)
- UART: USART2 (PA2=TX, PA3=RX) @ 115200 baud
- ADC: PA0 (analog input, 0-3.3V)
- PWM: PA1 (TIM2_CH2)
- LED: PC13 (active LOW)

### Firmware

The STM32 firmware lives in `../firmware/application/src/main.c` within this monorepo. It handles UART and ADC via DMA, with optimized interrupts and a lightweight JSON parser.

```bash
# Build via STM32CubeIDE or a Makefile generated by CubeMX,
# then flash with st-link:
st-flash write firmware.bin 0x8000000
```

---

## Usage

### Quick start

1. Plug in the STM32 via USB (UART-to-USB adapter)
2. Launch `./STM32Interface`
3. Select the serial port (e.g. `/dev/ttyACM0` or `COM3`)
4. Click "Connect"
5. Use the quick commands or send your own

### Commands

#### Text mode (legacy)

```
GET_TEMP         - Read temperature
GET_VOLTAGE      - Read ADC voltage
GET_ADC_RAW      - Raw ADC value
STATUS           - Full system state
SET_LED=0/1      - LED control
SET_PWM=0-100    - PWM duty cycle (%)
TOGGLE_LED       - Toggle LED state
RESET            - Reset the microcontroller
```

#### JSON mode (recommended)

**Read temperature:**
```json
{
  "type": "cmd",
  "command": "GET_TEMP"
}
```

**Response:**
```json
{
  "type": "response",
  "data": {
    "temp": 25.5
  }
}
```

**LED control:**
```json
{
  "type": "cmd",
  "command": "SET_LED",
  "params": {
    "state": 1
  }
}
```

**PWM control:**
```json
{
  "type": "cmd",
  "command": "SET_PWM",
  "params": {
    "duty": 75
  }
}
```

**Full status:**
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

## API examples

### Using DeviceController

```cpp
#include "DeviceController.h"

// create the controller
DeviceController *controller = new DeviceController(this);

// connect to the device
controller->connectToDevice("/dev/ttyACM0", 115200);

// send commands
controller->setLed(true);
controller->setPwm(50);
controller->requestTemperature();

// receive data
connect(controller, &DeviceController::temperatureUpdated,
        [](float temp) {
    qDebug() << "Temperature:" << temp << "°C";
});

// access the model
DeviceState *state = controller->deviceState();
qDebug() << "Current temp:" << state->temperature();

DataModel *dataModel = controller->dataModel();
auto tempHistory = dataModel->getTemperatureHistory();
```

### Custom threading

```cpp
// SerialWorker runs in its own thread,
// so I/O never blocks the UI.

SerialManager *serial = new SerialManager(this);

// async send
serial->sendCommand(data);  // non-blocking

// receive on the main thread
connect(serial, &SerialManager::dataReceived,
        this, &MyClass::handleData);
```

---

## Tech stack

| Area | Tech | Use |
|------|------|-----|
| Language | C++17 | Qt application |
| Framework | Qt 5.15+ | GUI |
| Architecture | MVC | Code organization |
| Threading | QThread | Async communication |
| Serialization | JSON | Wire format |
| Communication | QSerialPort | Serial driver |
| Build system | CMake | Compilation |
| Embedded | C + HAL | STM32 firmware |
| DMA | STM32 DMA | Efficient transfers |
| VCS | Git | Versioning |

---

## 🧪 Tests

Unit tests use QtTest.

```bash
cmake -DBUILD_TESTS=ON ..
make
ctest --output-on-failure
```

Individual test binaries:

```bash
./tests/test_jsonprotocol
./tests/test_devicestate
```

---

## 📚 Documentation

API documentation is generated with Doxygen from `desktop/Doxyfile`:

```bash
cd desktop
doxygen Doxyfile
```

The HTML output lands in `docs/doxygen/html/index.html`.

### Source layout

```
desktop/
├── CMakeLists.txt
├── Doxyfile
├── README.md
├── LICENSE
│
├── src/
│   ├── main.cpp
│   ├── model/
│   │   ├── DeviceState.{h,cpp}
│   │   └── DataModel.{h,cpp}
│   ├── view/
│   │   └── MainWindow.{h,cpp,ui}
│   ├── controller/
│   │   └── DeviceController.{h,cpp}
│   └── communication/
│       ├── SerialManager.{h,cpp}
│       ├── SerialWorker.{h,cpp}
│       └── JsonProtocol.{h,cpp}
├── qml/
│   ├── main.qml
│   └── resources.qrc
└── tests/
    ├── CMakeLists.txt
    ├── test_jsonprotocol.cpp
    └── test_devicestate.cpp
```

---

## Contributing

Contributions are welcome. The usual flow:

1. Fork the project
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes
4. Push the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## Troubleshooting

### Serial port not detected

```bash
# Linux: check permissions
sudo usermod -a -G dialout $USER
# Log out and back in — otherwise the new group is not picked up.

# List available ports
ls -l /dev/tty*
```

### Qt compilation error

```bash
qmake --version

# Reinstall Qt SerialPort if needed
sudo apt install libqt5serialport5-dev
```

### DMA not working (STM32 side)

- Check the DMA clock configuration
- Make sure the DMA interrupts are enabled
- Review the interrupt priorities

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## Author

**Bechir Yengui** — [github.com/BechirYengui](https://github.com/BechirYengui)
