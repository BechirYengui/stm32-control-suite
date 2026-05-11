# STM32 Control Suite

[![Predecessor: qt_interface_stm32](https://img.shields.io/badge/Predecessor-qt__interface__stm32-blue?style=flat-square&logo=github)](https://github.com/BechirYengui/qt_interface_stm32)
[![Predecessor: stm32_bootloader-application](https://img.shields.io/badge/Predecessor-stm32__bootloader--application-blue?style=flat-square&logo=github)](https://github.com/BechirYengui/stm32_bootloader-application)
[![Active development since](https://img.shields.io/badge/Active%20development-September%202025-green?style=flat-square)](https://github.com/BechirYengui/stm32-control-suite)

End-to-end embedded system combining a secure STM32 firmware and a C++/Qt desktop interface for real-time monitoring and control.

## Overview

Active development since September 2025 — consolidated in May 2026 from two predecessor repositories: [qt_interface_stm32](https://github.com/BechirYengui/qt_interface_stm32) and [stm32_bootloader-application](https://github.com/BechirYengui/stm32_bootloader-application) (both archived). See [Project History](#project-history) for details.

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

This monorepo is the consolidation of two repositories actively developed since September 2025 as part of my final-year apprenticeship at VEDECOM and post-apprenticeship personal projects:

- **[qt_interface_stm32](https://github.com/BechirYengui/qt_interface_stm32)** (archived May 2026) — Qt desktop interface for STM32 control. Initial commits: September 2025.
- **[stm32_bootloader-application](https://github.com/BechirYengui/stm32_bootloader-application)** (archived May 2026) — STM32 secure bootloader with AES-128-CBC, HMAC-SHA256, and anti-replay protection. Initial commits: September 2025.

The current monorepo (May 2026) unifies both subsystems under a shared structure with consistent build tooling and documentation. The archived repositories remain publicly accessible and preserve the original development history.

## License

MIT — see [LICENSE](LICENSE).

## Author

**Bechir Yengui** — [github.com/BechirYengui](https://github.com/BechirYengui)
