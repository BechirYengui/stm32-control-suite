# STM32 Control Suite

End-to-end embedded system combining a secure STM32 firmware and a C++/Qt desktop interface for real-time monitoring and control.

## Overview

This monorepo contains two complementary projects:

- **`firmware/`** — STM32F103 secure firmware: bootloader with CRC32 integrity verification, and application layer with AES-128-CBC encryption, HMAC-SHA256 authentication, and anti-replay protection.
- **`desktop/`** — C++/Qt graphical interface to control and monitor the STM32 in real time over a serial link, with MVC architecture and JSON-based protocol.

## Architecture

```
┌─────────────────────────┐         ┌──────────────────────────┐
│    Desktop (Qt / C++)   │  UART   │      STM32F103           │
│                         │ <─────> │                          │
│  - MVC architecture     │  JSON   │  - Secure bootloader     │
│  - QThread serial I/O   │ 115200  │  - Encrypted app layer   │
│  - Real-time dashboards │         │  - Peripherals (LED/PWM) │
└─────────────────────────┘         └──────────────────────────┘
```

## Repository Structure

```
stm32-control-suite/
├── firmware/                 # STM32 firmware (C, PlatformIO)
│   ├── bootloader/           # Secure bootloader (8 KB)
│   └── application/          # Encrypted application (48 KB)
├── desktop/                  # Qt C++ control interface
│   ├── src/                  # MVC source code
│   └── docs/                 # Architecture & migration notes
└── docs/                     # Global documentation
    ├── screenshots/
    └── diagrams/
```

## Tech Stack

| Layer | Technologies |
|-------|--------------|
| Desktop | C++17, Qt 5.15+, CMake, QSerialPort, QThread, JSON |
| Firmware | C, STM32 HAL, PlatformIO, mbedTLS, AES-128-CBC, HMAC-SHA256 |
| Protocol | UART 115200 bps, JSON framing, encrypted payload |

## Getting Started

See the dedicated READMEs:

- [Desktop application](desktop/README.md)
- [STM32 firmware](firmware/README.md)

## Project Highlights

- **Modern C++ architecture**: MVC, RAII, STL, smart pointers, multi-threading
- **Embedded security**: secure boot chain, encrypted communication, replay protection
- **Production-grade tooling**: CMake, PlatformIO, unit tests, Doxygen documentation
- **Cross-disciplinary**: hardware-level firmware to desktop UI, end-to-end ownership

## Project History

This monorepo consolidates two previously separate repositories developed between September 2025 and March 2026:

- **[qt_interface_stm32](https://github.com/BechirYengui/qt_interface_stm32)** (archived) — Original Qt desktop interface project
- **[stm32_bootloader-application](https://github.com/BechirYengui/stm32_bootloader-application)** (archived) — Original STM32 secure firmware project

Both repositories remain publicly accessible in archived state for reference. The current monorepo provides a unified structure, shared documentation, and consistent build tooling across both subsystems.

## License

MIT — see [LICENSE](LICENSE).

## Author

**Bechir Yengui** — [github.com/BechirYengui](https://github.com/BechirYengui)
