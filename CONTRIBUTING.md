# Contributing to ESP32 Multi-Protocol BMS Gateway

Thank you for your interest in contributing. This document covers everything needed to build the project from source, understand the code conventions, and submit a pull request.

---

## Table of Contents

- [Build Prerequisites](#build-prerequisites)
- [Getting Started](#getting-started)
- [Project Structure](#project-structure)
- [Code Style & Conventions](#code-style--conventions)
- [Pull Request Checklist](#pull-request-checklist)
- [Reporting Issues](#reporting-issues)

---

## Build Prerequisites

| Tool | Version | Notes |
| :--- | :--- | :--- |
| [Visual Studio Code](https://code.visualstudio.com/) | Latest | Required IDE |
| [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) | Latest | Manages toolchain, libraries, and uploads |
| Python | 3.8+ | Required by PlatformIO and `esptool.py` |
| Git | Any | For cloning and version control |
| USB driver | CP210x or CH340 | Depends on your ESP32-S3 board variant |
| Chrome or Edge | Latest | Required for the browser flash tool (WebSerial API) |

> PlatformIO downloads the ESP32-S3 toolchain and all library dependencies automatically on first build. No manual SDK installation is needed.

---

## Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol.git
cd ESP32-BMS-Gateway-Multi-Protocol
```

### 2. Create your Wi-Fi credentials file

Copy the example secrets file and fill in your network details:

```bash
cp include/secrets.h.example include/secrets.h
```

Edit `include/secrets.h`:

```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASS "YourPassword"
```

> `secrets.h` is excluded from version control via `.gitignore`. Never commit real credentials.

### 3. Erase flash (first use or after partition changes)

In VS Code with PlatformIO:

```
PlatformIO sidebar → Platform → Erase Flash
```

This is mandatory on a new board or after any change to `board_build.partitions`.

### 4. Build and upload the filesystem

The web dashboard assets in `data/` are packaged into a LittleFS image:

```
PlatformIO sidebar → Platform → Build Filesystem Image
PlatformIO sidebar → Platform → Upload Filesystem Image
```

### 5. Build and upload the firmware

```bash
pio run --target upload
```

Or use the **Upload** button (→) in the PlatformIO toolbar.

### 6. Open the Serial Monitor

```
Monitor speed: 115200 baud
```

Boot logs confirm Wi-Fi status, active IP address, and protocol initialisation order.

---

## Project Structure

```
ESP32-BMS-Gateway-Multi-Protocol/
│
├── src/
│   ├── main.cpp                ← Orchestration: setup(), loop(), Wi-Fi, mDNS, Safe Reboot
│   ├── state.cpp               ← Single source of truth for all BMS points (AnalogPoint, BinaryPoint)
│   ├── bacnet_handler.cpp      ← BACnet/IP stack integration, object callbacks, UDP routing
│   ├── modbus_handler.cpp      ← Modbus TCP register map ↔ shared state synchronisation
│   └── web_handler.cpp         ← REST API (11 endpoints), LittleFS, point CRUD, network config
│
├── include/
│   ├── config.h                ← Hardware notes, BACnet and Modbus address constants
│   ├── state.h                 ← BinaryPoint / AnalogPoint / DashboardSection structs
│   ├── bacnet_handler.h        ← bacnet_init(), bacnet_task(), bacnet_update_objects()
│   ├── modbus_handler.h        ← modbus_init(), modbus_task(), modbus_sync_from_registers()
│   ├── web_handler.h           ← web_server_init(), web_server_task()
│   └── secrets.h.example       ← Template for Wi-Fi credentials (copy to secrets.h)
│
├── data/                       ← Front-end assets (packaged into LittleFS)
│   ├── index.html              ← SPA shell
│   ├── style.css               ← ISA 101 compliant stylesheet
│   └── app.js                  ← Real-time polling, sessionStorage cache, conflict detection
│
├── lib/
│   └── bacnet/                 ← Vendored BACnet stack (GPL v2+), optimised for ESP32
│
├── docs/                       ← GitHub Pages: browser flash interface + release binaries
│   ├── index.html              ← ESP Web Tools flash page
│   ├── manifest.json           ← Firmware manifest for ESP Web Tools
│   ├── merged-firmware.bin     ← Combined bootloader + firmware + LittleFS image
│   ├── littlefs.bin            ← LittleFS filesystem image
│   ├── boot_app0.bin           ← OTA data partition (required by esptool merge_bin)
│   ├── BUILD_RELEASE.md        ← Release binary build procedure
│   └── PARTITION_LAYOUT.md     ← Partition table with offsets and sizes
│
├── CHANGELOG.md                ← Version history
├── CONTRIBUTING.md             ← This file
├── platformio.ini              ← Build configuration
└── LICENSE                     ← MIT (firmware) + GPL v2+ (lib/bacnet)
```

---

## Code Style & Conventions

### Language

- All source files are **C++** (Arduino framework, ESP32-S3 target).
- All comments, Doxygen blocks, log strings, and variable names must be in **English**.
- Use the **imperative present tense** for inline comments: `// Flush the TCP socket before restarting` not `// Flushing...` or `// This flushes...`.

### Formatting

- Indentation: **4 spaces** (no tabs).
- Maximum line length: **120 characters**.
- Braces: **Allman style** for functions, **K&R** acceptable for single-statement conditionals.
- Use `strncpy` instead of `strcpy`; always respect `MAX_NAME_LEN` bounds.
- Use `static_cast<>` instead of C-style casts.

### Doxygen

Every **public function** declared in a header file must have a complete Doxygen block:

```cpp
/**
 * @brief  One-line imperative summary.
 * @param  paramName  What this parameter represents and its valid range.
 * @return What is returned and what each value means. Omit for void functions.
 */
```

Internal (file-scope) functions that are not in headers need at minimum a `/** @brief ... */` one-liner above the function definition.

### Architecture rules

- **Single source of truth:** all point mutations go through `state.cpp` arrays (`binaryPoints[]`, `analogPoints[]`). Do not duplicate point state in protocol handlers.
- **No cross-handler coupling:** `bacnet_handler.cpp`, `modbus_handler.cpp`, and `web_handler.cpp` must not call each other directly. All communication goes through `state.cpp`.
- **Safe Reboot pattern:** never call `ESP.restart()` inside a web server callback. Set `pendingReboot = true` and let `loop()` handle it.
- **No heap allocation in `loop()`:** all dynamic allocation must happen during initialisation (`setup()`).

### Module toggles

The three protocol modules can be individually disabled via build flags in `platformio.ini`:

```ini
-D ENABLE_MODBUS=1   ; set to 0 or remove to disable
-D ENABLE_BACNET=1
-D ENABLE_WEB=1
```

New protocol-specific code must always be wrapped in the corresponding `#ifdef`.

---

## Pull Request Checklist

Before opening a PR, verify every item below:

- [ ] **Zero compiler warnings** — build with `-Wall -Wextra` produces no new warnings.
- [ ] **All comments in English** — no French or other languages in source files.
- [ ] **Doxygen complete** — every new or modified public function has `@brief`, `@param`, and `@return` (where applicable).
- [ ] **Single source of truth respected** — no direct state duplication between handlers.
- [ ] **`#ifdef` guards applied** — all new protocol-specific code is wrapped in the appropriate compile-time flag.
- [ ] **Tested on hardware** — the change has been flashed and verified on a physical ESP32-S3.
- [ ] **`CHANGELOG.md` updated** — add an entry under `[Unreleased]` describing what was added, changed, or removed.
- [ ] **No credentials committed** — `secrets.h` must not appear in the diff.

---

## Reporting Issues

Open an issue on [GitHub Issues](https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/issues) and include:

1. Firmware version (visible in the top bar of the web dashboard).
2. Board variant (flash size, PSRAM).
3. Which protocol is affected (BACnet, Modbus, Web UI).
4. Serial monitor output at 115200 baud covering the boot sequence and the failure.
5. Steps to reproduce.
