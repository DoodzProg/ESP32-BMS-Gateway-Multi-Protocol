# ESP32 Multi-Protocol BMS Gateway

[![Version](https://img.shields.io/badge/Version-v1.3.0-brightgreen.svg)](https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/releases/tag/v1.3.0)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20%7C%20PlatformIO-00979D.svg)](https://platformio.org/)
[![BACnet/IP](https://img.shields.io/badge/BACnet%2FIP-Port_47808-4A90D9.svg)](https://github.com/bacnet-stack/bacnet-stack)
[![Modbus TCP](https://img.shields.io/badge/Modbus_TCP-Port_502-D94A4A.svg)](https://modbus.org/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)
[![Flash Tool](https://img.shields.io/badge/Flash-ESP_Web_Tools-orange.svg)](https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol)

> **A $10 industrial protocol bridge — Modbus TCP ↔ BACnet/IP — on a single ESP32-S3.**  
> Flash it from your browser in 90 seconds. No IDE, no drivers, no middleware, no cloud.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Install — Browser Flash](#quick-install--browser-flash)
- [Developer Setup — Build from Source](#developer-setup--build-from-source)
- [Hardware Requirements](#hardware-requirements)
- [Usage & Configuration](#usage--configuration)
- [Architecture](#architecture)
- [Protocol Reference](#protocol-reference)
- [REST API Reference](#rest-api-reference)
- [Security](#security)
- [Developer Documentation](#developer-documentation)
- [License & Credits](#license--credits)

---

## Overview

The **ESP32 Multi-Protocol BMS Gateway** is open-source firmware that transforms an ESP32-S3 into a fully bidirectional industrial gateway. It simultaneously serves data over **BACnet/IP** and **Modbus TCP** while hosting an embedded **High Performance HMI** (ISA 101 compliant) directly from device flash.

**Core use case:** your equipment speaks Modbus TCP (a VFD, a sensor, a PLC output). Your building supervisor speaks BACnet/IP (EBO, Desigo CC, SCADA, JACE). This device sits between them, translates in real time, and costs under $10 in hardware.

**Typical applications:**

- Connecting HVAC drives or field devices to a BACnet/IP supervisor
- Exposing Modbus TCP slave data to a BACnet network without a PC-based gateway
- Local commissioning and monitoring via an embedded web dashboard
- Rapid BMS point map prototyping before production deployment

---

## Features

### Plug & Play

- Flash directly from your browser using [ESP Web Tools](https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol) — zero IDE, zero drivers
- Automatic Wi-Fi AP fallback (`BMS-Gateway-Config`) when no saved network is reachable
- Device accessible on the LAN at `http://bms-<name>.local` via mDNS — no IP address to memorize

### Industrial Web HMI

- ISA 101 High Performance HMI compliant interface: desaturated palette, SVG icons, color contrast reserved for operational states (RUN/STOP) and alarms only
- Single-Page Application (SPA) served directly from ESP32 flash — near-instant page loads
- Live point configuration: Modbus register / BACnet object mapping with real-time address conflict detection
- **Live Log Viewer** — built-in terminal panel; Server-Sent Events stream delivers system, BACnet, and web events in real time
- Front-end value caching via `sessionStorage` — display does not blank during a device reboot

### BACnet/IP — BTL-Ready Conformance

- **SubscribeCOV** — supervisors subscribe to AV/BV objects; unsolicited `ConfirmedCOVNotification` sent on value change exceeding the COV-Increment (default 1.0 EU, overridable via `WriteProperty`)
- **ReadPropertyMultiple (RPM)** — single UDP exchange reads all object properties at once; matches the standard BAS integrator discovery workflow used by Desigo CC, EBI, and JACE
- **Full Device Object** — all 20 BTL-required properties implemented; Vendor Identifier = 260; zero "unknown vendor" warnings in YABE or any compliant supervisor
- Up to **64 AV / 64 BV / 128 objects** — stable under continuous polling from professional supervisors

### Protocol Engine

- BACnet/IP and Modbus TCP run concurrently with no interference
- All point configurations persist to `/config.json` on LittleFS and survive power cuts
- Wi-Fi credentials and AP/STA mode override saved to NVRAM — survive OTA and reboots
- PSRAM-backed JSON document pool prevents heap fragmentation under heavy polling
- Atomic `config.json` write via temp file + rename — no corruption on power loss during save

### Security (v1.3.0)

- HTTP Basic Authentication — SHA-256 password hash stored in NVS; default-password banner until credentials are changed
- Credentials never exposed in `/api/system` responses — boolean flags only (`hasSTAPass`, `hasAPPass`)
- Input validation on all user-supplied names: whitelist `[a-zA-Z0-9_\-]`, server-side enforced
- Auth required on all API endpoints including `/api/scan`
- Wi-Fi credentials transmitted in POST body only — never in URL query parameters

---

## Quick Install — Browser Flash

No development environment required. The full flash takes under 90 seconds.

### 1 — Open the Flash Page

**[https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol](https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol)**

<div align="center">
  <img src="assets/QRcode_Installer-web.png" alt="QR Code for Web Installer" width="150"/>
</div>

> **Browser requirement:** Google Chrome or Microsoft Edge (desktop). ESP Web Tools requires the WebSerial API, which Firefox does not currently support.

### 2 — Connect and Flash

1. Plug your ESP32-S3 into a USB port.
2. Click **Install** on the flash page and select your COM port from the browser dialog.
3. Wait for the process to complete (approximately 60–90 seconds). The device reboots automatically.

> On a new board or after any partition layout change, perform a full **Erase Flash** first. This option is available directly on the flash page.

### 3 — First Boot

On first boot, or whenever the saved Wi-Fi network is unreachable, the device creates a local Access Point:

| Parameter | Value |
| :--- | :--- |
| SSID | `BMS-Gateway-Config` |
| Password | `admin1234` |
| Fallback IP | `192.168.4.1` |

Connect to this network, then open `http://bms-gateway.local` or `http://192.168.4.1` in your browser.

> **Change the default password immediately** before deploying in any shared or production environment.

### 4 — Join Your Network

1. Open **Network Configuration** in the dashboard.
2. Click **Scan**, select your Wi-Fi network, and enter the passphrase.
3. Click **Apply & Reboot**. Credentials are saved to NVRAM.

After reconnecting, BACnet/IP (UDP `47808`) and Modbus TCP (TCP `502`) are immediately available on your LAN. The dashboard is reachable at `http://bms-<name>.local` from any browser on the same subnet.

---

## Developer Setup — Build from Source

Use this path to modify the firmware, add custom data points, or contribute to the project.

**Prerequisites:** [Visual Studio Code](https://code.visualstudio.com/) + [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

### 1 — Clone

```bash
git clone https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol.git
cd ESP32-BMS-Gateway-Multi-Protocol
```

### 2 — Open in PlatformIO

Open the cloned folder in VS Code. PlatformIO detects `platformio.ini` and resolves all dependencies automatically.

### 3 — Erase Flash (First Use on a New Board)

```
PlatformIO sidebar → Platform → Erase Flash
```

Mandatory on any new board or after changing the partition layout.

### 4 — Build and Upload the Filesystem

The web dashboard lives in `data/` and is packaged into a LittleFS image:

```
PlatformIO sidebar → Platform → Build Filesystem Image
PlatformIO sidebar → Platform → Upload Filesystem Image
```

### 5 — Build and Upload the Firmware

```bash
pio run --target upload
```

Or use the **Upload** button (→) in the PlatformIO toolbar.

### 6 — Monitor Serial

Open the Serial Monitor at **115200 baud** to observe boot logs, Wi-Fi connection status, IP address assignment, BACnet initialization, and all runtime events.

### Adding a Custom Data Point

Declare the point in `state.cpp`. Both the BACnet and Modbus handlers enumerate the state registry at runtime — no other files require modification.

### Building Release Binaries

See **[docs/BUILD_RELEASE.md](docs/BUILD_RELEASE.md)** for the full procedure to produce the `merged-firmware.bin` and `littlefs.bin` files used by the GitHub Pages browser flash tool.

---

## Hardware Requirements

### Recommended Target

| Parameter | Specification |
| :--- | :--- |
| **SoC** | ESP32-S3 |
| **Flash** | 16 MB (N16R8 variant recommended) |
| **PSRAM** | 8 MB OPI (optional but recommended at 50+ points) |
| **USB** | USB-C with data lines (not charge-only) |

Firmware is built and tested on the **ESP32-S3 N16R8** (YD-ESP32-S3). This variant provides 16 MB flash for the LittleFS partition, the application binary, and OTA slots with comfortable headroom. OPI PSRAM (`qio_opi` memory type) is activated by the build configuration and backed by `ps_malloc()` for large JSON documents.

### Minimum Viable Hardware

Any ESP32-S3 development board with at least **8 MB of flash** will run the firmware. The LittleFS partition is flashed at address `0x670000` and requires approximately 1.5 MB of flash beyond the application partition.

> Standard ESP32 (non-S3) and ESP32-S2 variants are **not supported** due to differences in the USB peripheral and memory layout used by this firmware.

---

## Usage & Configuration

### AP Fallback

When the gateway cannot reach its saved Wi-Fi network (first boot, network down, credentials changed), it automatically activates its fallback Access Point within 30 seconds.

Connect to `BMS-Gateway-Config` (password: `admin1234`) and open `http://bms-gateway.local` or `http://192.168.4.1`.

### Configuring BMS Points

From the dashboard:

1. Switch to **Edit Mode** using the toggle in the top bar.
2. For each data point, assign a **name**, a **Modbus address** (Holding Register or Coil), and a **BACnet object instance** (Analog Value or Binary Value).
3. The interface checks for address conflicts in real time.
4. Click **Save Configuration**. The device performs a clean Safe Reboot to apply the new point map.

Configuration is written atomically to `/config.json` on LittleFS (via temp file + rename). It persists through power cuts, OTA updates, and reboots.

### Default Point Map (AHU Simulator)

The firmware ships with a pre-configured Air Handling Unit simulator for immediate integration testing:

| Point | BACnet Object | Modbus Address | Description | Range |
| :--- | :---: | :---: | :--- | :--- |
| Fan Status | BV:0 | Coil 10 | Fan run/stop command | 0 = Stop, 1 = Run |
| Fan Speed | AV:0 | Holding Reg. 100 | VSD output speed | 0–100 % |
| Temp Setpoint | AV:1 | Holding Reg. 101 | Supply air setpoint | ×10 scale — `210` = 21.0 °C |

### Web Interface Navigation

| Section | Description |
| :--- | :--- |
| **Dashboard — View Mode** | Live view of all data points, gauges, and current values |
| **Dashboard — Edit Mode** | Add, edit, delete, and reorder points and sections |
| **Logs** | Real-time system and protocol event log; SSE stream with connection status indicator |
| **Network Configuration** | Wi-Fi scan and connection, AP/STA mode override, device name, password change |

---

## Architecture

```
ESP32-BMS-Gateway-Multi-Protocol/
│
├── src/
│   ├── main.cpp                ← Orchestration: setup(), loop(), task scheduling, Safe Reboot
│   ├── state.cpp               ← Centralized point registry (AnalogPoint, BinaryPoint)
│   ├── bacnet_handler.cpp      ← BACnet/IP stack, COV/RPM handlers, Device Object stubs
│   ├── modbus_handler.cpp      ← Modbus TCP register map ↔ shared state synchronization
│   ├── web_handler.cpp         ← REST API, log stream, network scan/connect, auth
│   ├── log_handler.cpp         ← In-RAM ring buffer (64 × 80 B), SSE delivery
│   └── sc_stubs.c              ← BACnet/SC linker stubs (bacapp_encode_SC*)
│
├── data/                       ← Front-end assets — packaged into LittleFS
│   ├── index.html              ← Single-Page Application shell
│   ├── style.css               ← ISA 101 compliant stylesheet
│   └── app.js                  ← Real-time polling, sessionStorage caching, conflict detection
│
├── lib/
│   └── bacnet/                 ← Vendored C BACnet stack, optimized for embedded targets
│
├── docs/                       ← GitHub Pages — web flash interface + release binaries
│   ├── index.html              ← ESP Web Tools installer page
│   ├── manifest.json           ← ESP Web Tools firmware manifest
│   ├── merged-firmware.bin     ← Full flash image (generated — not committed to git)
│   ├── littlefs.bin            ← Web UI filesystem image (generated — not committed to git)
│   ├── BUILD_RELEASE.md        ← Release binary build procedure
│   └── PARTITION_LAYOUT.md     ← Partition table with offsets and sizes
│
├── CHANGELOG.md                ← Full version history
├── CONTRIBUTING.md             ← Build guide, code conventions, PR checklist
└── platformio.ini              ← Build config: flash layout, partition table, build flags
```

### Data Flow

All three interfaces share a single state registry. A write from any source — BACnet supervisor, Modbus master, or web UI — is immediately visible on all others with no translation layer.

```
BACnet/IP Supervisor ──► bacnet_handler.cpp ──┐
                                               │
Modbus TCP Master    ──► modbus_handler.cpp ──►│──► state.cpp (shared registry) ──► all interfaces
                                               │
Web Dashboard        ──► web_handler.cpp    ──┘
```

---

## Protocol Reference

### BACnet/IP

| Parameter | Value |
| :--- | :--- |
| Transport | UDP, port `47808` |
| Supported services | `ReadProperty`, `WriteProperty`, `ReadPropertyMultiple`, `SubscribeCOV`, `Who-Is` / `I-Am` |
| Object types | Analog Value (AV), Binary Value (BV), Device Object |
| Vendor Identifier | `260` (BTL-ready) |
| Capacity | 64 AV, 64 BV, 128 objects maximum |
| COV-Increment | 1.0 EU default (overridable per-object via `WriteProperty`) |
| Discovery | Responds to broadcast `Who-Is` with full Device Object property set — zero manual configuration |

### Modbus TCP

| Parameter | Value |
| :--- | :--- |
| Transport | TCP, port `502` |
| FC01 | Read Coils |
| FC05 | Write Single Coil |
| FC03 | Read Holding Registers |
| FC06 | Write Single Register |
| FC16 | Write Multiple Registers |
| Addressing | 0-based for both coils and holding registers |

---

## REST API Reference

All endpoints are authenticated (HTTP Basic Auth) and accept/return `application/json`. The AP fallback interface bypasses authentication to allow captive-portal configuration.

| Method | Endpoint | Description |
| :---: | :--- | :--- |
| `GET` | `/api/data` | Live telemetry values + heap/PSRAM stats |
| `GET` | `/api/system` | Network state, IP addresses, device name, auth flags |
| `GET` | `/api/config` | Full point definitions and dashboard layout |
| `POST` | `/api/config/layout` | Persist section order and sizes |
| `POST` | `/api/point/add` | Create a new analog or binary point |
| `POST` | `/api/point/update` | Update an existing point's metadata |
| `POST` | `/api/point/delete` | Remove a point and its section membership |
| `POST` | `/api/point/write` | Write a live value to a point |
| `GET` | `/api/point/check` | Validate address availability before committing |
| `GET` | `/api/scan` | Start or poll an asynchronous Wi-Fi network scan |
| `POST` | `/api/switch_network` | Persist network credentials and trigger reboot |
| `POST` | `/api/auth/change` | Change the web UI password |
| `POST` | `/api/device/name` | Set device name (mDNS hostname + AP SSID prefix) |
| `GET` | `/api/log/stream` | Server-Sent Events stream of the in-RAM log ring buffer |

---

## Security

### Authentication

Web UI access is protected by HTTP Basic Authentication. The password hash (SHA-256) is stored in NVS — never in plaintext. The default password (`admin1234`) triggers a visible banner on the dashboard until changed.

The AP captive portal intentionally bypasses authentication to allow first-boot configuration from a fresh device. Once connected to an enterprise network (STA mode), all endpoints require credentials.

### Input Validation

All user-supplied names (point names, device name) are validated server-side against the whitelist `[a-zA-Z0-9_\-]` (1–63 chars for points, 1–32 chars for device name). Requests with invalid names are rejected with HTTP 400 before any NVS write or state mutation occurs.

### Credentials in Transit

Wi-Fi and AP credentials are transmitted exclusively in POST request bodies (JSON) — never as URL query parameters, never logged to serial.

### `/api/system` Response

The system endpoint returns boolean flags (`hasSTAPass`, `hasAPPass`) instead of credential values. Stored passwords are never read from NVS in response handlers.

---

## Developer Documentation

| Document | Description |
| :--- | :--- |
| [CHANGELOG.md](CHANGELOG.md) | Full version history — what changed and why in every release |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Build prerequisites, code style rules, and pull-request checklist |
| [docs/BUILD_RELEASE.md](docs/BUILD_RELEASE.md) | Step-by-step procedure for building and publishing release binaries |
| [docs/PARTITION_LAYOUT.md](docs/PARTITION_LAYOUT.md) | Flash partition table with offsets, sizes, and role descriptions |

---

## License & Credits

This project is released under the **MIT License** — see [LICENSE](LICENSE) for full terms.

**Author:** [Doodz (DoodzProg)](https://github.com/DoodzProg)

**Third-party components:**

[`bacnet-stack`](https://github.com/bacnet-stack/bacnet-stack) — Licensed under **GNU GPL v2 or later**. The vendored BACnet library in `lib/bacnet/` retains its original GPL license. Distributions of modified versions must comply with the GPL terms for that component.

---

<div align="center">

*Built for the field. Tested with YABE and qModMaster.*  
*Made by Doodz with the assistance of [Anthropic Claude Code](https://claude.ai/code) AI — and a lot of coffee.*

**[⚡ Flash Now — No Setup Required](https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol)**

</div>
