# STM32 Control Suite

End-to-end embedded control stack for an STM32F103: a secure-boot firmware
on the device and a Qt/C++ desktop application that drives it over a
serial link.

## Repository layout

```
.
├── firmware/   STM32F103 bootloader, application image and crypto library
│   ├── bootloader/   Secure-boot stage at 0x08000000 (CRC32 + SHA-256 image check)
│   └── application/  Main application at 0x08002000 (UART/DMA, ADC, PWM, JSON + text protocol)
│
├── desktop/    Qt 5 desktop client (C++17, MVC, threaded serial worker)
│
└── docs/       Project-wide diagrams and screenshots
```

- Firmware module: see [`firmware/README.md`](firmware/README.md)
- Desktop module: see [`desktop/README.md`](desktop/README.md)

## Project History

This monorepo consolidates two previously separate repositories developed
between September 2025 and March 2026:

- **[qt_interface_stm32](https://github.com/BechirYengui/qt_interface_stm32)**
  (archived) — Original Qt desktop interface project.
- **[stm32_bootloader-application](https://github.com/BechirYengui/stm32_bootloader-application)**
  (archived) — Original STM32 secure firmware project.

Both repositories remain publicly accessible in archived state for
reference. The current monorepo provides a unified structure, shared
documentation, and consistent build tooling across both subsystems.

## License

This project is released under the MIT License. See [LICENSE](LICENSE)
for details.
