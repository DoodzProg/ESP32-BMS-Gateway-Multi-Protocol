# Changelog

All notable changes to this project are documented in this file.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [1.0.1] — 2026-04-08

### Added
- `CHANGELOG.md` — this file; tracks all version history going forward.
- `CONTRIBUTING.md` — build prerequisites, code style rules, and pull-request checklist.
- `docs/BUILD_RELEASE.md` — step-by-step procedure for building and publishing release binaries.
- `docs/PARTITION_LAYOUT.md` — full partition table with offsets, sizes, and role descriptions.
- `-Wall -Wextra` compiler flags in `platformio.ini` with `‑Wno‑unused‑parameter` to suppress expected Arduino framework noise.
- Doxygen `@brief` on all internal HTTP handler functions in `web_handler.cpp`.
- Hardware notes block in `config.h` documenting LED behaviour and BOOT button (GPIO 0).

### Changed
- Version bumped to **v1.0.1** across all source file headers, `platformio.ini`, the web dashboard badge, and `README.md`.
- API Surface comment in `web_handler.cpp` updated to `v1.0.1`.

### Removed
- LED status logic: `LED_PIN` define, `pinMode`/`digitalWrite` calls, and the hardware-feedback block in `loop()`.
  Rationale: the YD-ESP32-S3 N16R8 board has no general-purpose controllable LED; system state is now observable exclusively via Serial logs and the Web UI.
- `#include "config.h"` from `main.cpp` (no longer needed after LED removal).

---

## [1.0.0] — 2026-04-04

### Added
- Browser-based flashing via [ESP Web Tools](https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol) — zero IDE, zero command line.
- GitHub Pages flash interface (`docs/index.html`, `docs/manifest.json`, merged firmware binary).
- Complete Doxygen headers on all public functions and data structures across `src/` and `include/`.
- Professional `README.md` with architecture diagram, protocol reference, and QR code installer link.

### Changed
- Production-ready release milestone; all core features stable and validated with YABE and professional BACnet supervisors.

---

## [0.3.0] — 2026-03-xx

### Added
- Persistent Wi-Fi credentials stored in NVRAM (survive OTA and power cycles).
- Explicit AP / STA mode toggle from the web UI.
- Automatic AP fallback (`BMS-Gateway-Config`) when STA connection fails within 10 seconds.
- mDNS registration — device accessible at `http://bms-gateway.local` with no IP lookup.
- `sessionStorage` front-end value cache — dashboard does not blank during device reboot.
- Click-to-copy for device and client IP addresses in the top bar.

### Changed
- Network panel redesigned with Wi-Fi scan, credential input, and mode-override controls.

---

## [0.2.0] — 2026-03-xx

### Added
- Full bidirectional gateway: writes from any source (BACnet, Modbus, Web UI) propagate immediately to all others.
- ISA 101 High Performance HMI compliant dashboard (desaturated palette, SVG icons, color reserved for operational states and alarms).
- Drag-and-drop section and point reordering with persistent layout saved to LittleFS.
- Live address-conflict detection in the point-configuration UI.
- Modbus scaling factor per analog point (e.g., ×10 for one decimal place over integer registers).
- Dark / light theme toggle with OS preference detection.

### Changed
- REST API expanded to 11 endpoints covering full point and layout CRUD.

---

## [0.1.0] — 2026-02-xx

### Added
- Initial release for ESP32-S3.
- Simultaneous **BACnet/IP** (UDP 47808) and **Modbus TCP** (TCP 502) server.
- Embedded web server on port 80 serving a Single-Page Application from LittleFS.
- Pre-configured AHU (Air Handling Unit) simulator point map: FanStatus (BV:0 / Coil 10), FanSpeed (AV:0 / Reg 100), TempSetpoint (AV:1 / Reg 101).
- Single-source-of-truth architecture: all points declared once in `state.cpp`, reflected automatically across all three protocol handlers.
- JSON configuration persistence to `/config.json` on LittleFS.
- Safe Reboot mechanism via `pendingReboot` flag to prevent watchdog crashes during web-triggered restarts.

[Unreleased]: https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/compare/v1.0.1...HEAD
[1.0.1]: https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/compare/v0.3.0...v1.0.0
[0.3.0]: https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/DoodzProg/ESP32-BMS-Gateway-Multi-Protocol/releases/tag/v0.1.0
