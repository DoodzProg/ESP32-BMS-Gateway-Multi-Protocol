# ESP32 Multi-Protocol BMS Gateway 🏗️

[![Version](https://img.shields.io/badge/Version-0.2.0-orange.svg)](https://github.com/DoodzProg/bacnet-modbus-esp32/releases)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/Framework-Arduino-00979D.svg)](https://www.arduino.cc/)
[![BACnet/IP](https://img.shields.io/badge/BACnet%2FIP-Port_47808-green.svg)](https://github.com/bacnet-stack/bacnet-stack)
[![bacnet-stack](https://img.shields.io/badge/bacnet--stack-v1.5.0.rc5-brightgreen.svg)](https://github.com/bacnet-stack/bacnet-stack)
[![Modbus TCP](https://img.shields.io/badge/Modbus_TCP-Port_502-red.svg)](https://modbus.org/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

> **Transform an ESP32-S3 into an active, bidirectional industrial gateway.**
> Simultaneously expose your data via **BACnet/IP**, **Modbus TCP**, and a real-time **Web Dashboard**.
> Features a modular, data-driven architecture.

---

## 🚀 Features

| Feature | Detail |
| :--- | :--- |
| 🟢 **Bidirectional BACnet/IP** | Read *and* write from YABE (`ReadProperty` + `WriteProperty`) |
| 🔴 **Modbus TCP Server** | Exposes Coils & Holding Registers (port `502`) |
| 🌐 **Embedded Web Dashboard** | Auto-generated asynchronous HTML/JS interface (port `80`) |
| ⚡ **Real-Time Synchronization** | A write via Modbus or BACnet instantly propagates to the other two interfaces |
| 🧩 **Modular Architecture** | Each protocol is isolated in its own handler; data is centralized in `state` |
| 🔧 **Conditional Compilation** | Enable/disable each service via flags in `platformio.ini` without modifying source code |

---

## 📐 Project Architecture

    bacnet-modbus-esp32/
    │
    ├── include/
    │   ├── config.h              ← Network and hardware settings (pins, IP, ports...)
    │   └── secrets.h             ← Wi-Fi credentials (to be created, gitignored)
    │
    ├── src/
    │   ├── main.cpp              ← Orchestration: setup, main loop, service initialization
    │   ├── state.cpp             ← Centralized data points definition (BinaryPoint, AnalogPoint)
    │   ├── bacnet_handler.cpp    ← BACnet/IP logic: objects, ReadProperty, WriteProperty
    │   ├── modbus_handler.cpp    ← Modbus TCP logic: register mapping ↔ shared state
    │   └── web_handler.cpp       ← Async Web Server: auto-generated HTML/JS dashboard
    │
    ├── lib/
    │   └── bacnet/               ← C BACnet Stack (bacnet-stack v1.5.0.rc5, manually imported)
    │       ├── datalink/
    │       │   └── bip-esp32.c   ← Custom UDP datalink layer for ESP32 (WiFiUDP ↔ C stack)
    │       └── library.json      ← PlatformIO config: excludes unused BACnet modules
    │
    ├── test/
    ├── platformio.ini            ← Build configuration, conditional compilation flags
    └── README.md

**How it works:** `state.cpp` defines arrays of `BinaryPoint` and `AnalogPoint` structures representing each data point. At startup, each handler loops through these arrays to automatically create the BACnet objects, Modbus registers, and Web dashboard cards. Any value change—regardless of the source—is written to this shared state and immediately propagated to the other interfaces.

---

## 📦 Default Use Case: AHU Simulator

The firmware includes an **Air Handling Unit (AHU)** simulator by default. It's a standard BMS equipment example that illustrates the three most common types of variables in building automation. The system is fully customizable via `state.cpp`.

| Point | BACnet Object | Modbus Register | Web Interface | Description |
| :--- | :---: | :---: | :---: | :--- |
| **Fan Status** | Binary Value: 0 | Coil: 10 | ON / OFF | Run / Stop |
| **Fan Speed** | Analog Value: 0 | Hreg: 100 *(101 on QModMaster)* | Speed (%) | 0 to 100 % |
| **Temp Setpoint** | Analog Value: 1 | Hreg: 101 *(102 on QModMaster)* | Temperature (°C) | Multiplier ×10 — e.g. `210` = 21.0 °C |

> **QModMaster offset note:** Some Modbus software applies a +1 offset to register addresses. The native firmware addresses remain those listed above.

---

## 🧰 Hardware Requirements & Compatibility

Developed and tested on the following board:

- **Model:** YD-ESP32-S3 N16R8 (8 MB PSRAM, 16 MB Flash)
- **Purchase Link:** [AliExpress — ESP32-S3 N16R8](https://fr.aliexpress.com/item/1005006266375800.html) *(Select "N16R8 with Cable")*

> **Compatibility with other ESP32-S3 boards:**
> The code works on generic N16R8 boards. The LED pin may vary depending on the manufacturer (configured to `16` here for the blue LED). If necessary, modify `#define LED_PIN` in `config.h`.
>
> *The "PSRAM ID read error" warning visible in the serial monitor at startup on some generic boards is harmless — it does not affect the firmware's operation.*

---

## ⚙️ Installation & Deployment

**1. Clone the repository**

    git clone https://github.com/DoodzProg/bacnet-modbus-esp32.git

**2. Configure Wi-Fi credentials**

For security reasons, passwords are not included in the repository. Create a `secrets.h` file in the `include/` folder:

    // include/secrets.h
    #ifndef SECRETS_H
    #define SECRETS_H
    
    #define WIFI_SSID "Your_Network_Name"
    #define WIFI_PASS "Your_Password"
    
    #endif

**3. Build & Flash**

Open the project in **VS Code** with the **PlatformIO** extension, then click `Upload` (➡️).

> *Troubleshooting:* On some generic boards, hold `BOOT`, press `RST` once, and release `BOOT` to force flash mode.

---

## 🛠️ Recommended Testing Tools

| Tool | Usage |
| :--- | :--- |
| **[YABE](https://sourceforge.net/projects/yetanotherbacnetexplorer/)** | Add your PC's IP, send a `Who-Is` — the `ESP32-CTA-Sim` node will appear. Allows reading and writing via `WriteProperty`. |
| **[QModMaster](https://sourceforge.net/projects/qmodmaster/)** | Connect to the ESP32 IP on port `502` to read and write Modbus registers. |
| **Web Browser** | Go to `http://<ESP32_IP>` for the real-time Dashboard. |

---

## 📚 Dependencies

| Library | Source | Version |
| :--- | :--- | :--- |
| **bacnet-stack** | [github.com/bacnet-stack/bacnet-stack](https://github.com/bacnet-stack/bacnet-stack) | `1.5.0.rc5` |
| **AsyncTCP** + **ESPAsyncWebServer** | Arduino / PlatformIO repositories | Latest stable |
| **ModbusIP_ESP8266** (ESP32 port) | PlatformIO registry | Latest stable |

---

## 🗺️ Roadmap

- Multi-device support (multiple AHUs in parallel)
- BACnet COV (Change Of Value) — push notifications
- Web configuration interface (SSID, points, mapping) without recompiling
- MQTT export to external broker (Home Assistant, Node-RED)

---

## 📄 License

This project is licensed under the **MIT** License. See the [LICENSE](LICENSE) file for details.

---

<div align="center">

*Made with ☕ and a lot of dropped UDP frames.*

</div>