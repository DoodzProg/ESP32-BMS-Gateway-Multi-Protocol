# Flash Partition Layout

This document describes the flash memory partition table used by the ESP32 BMS Gateway firmware.

**Partition table file:** `default_8MB.csv` (from the ESP32 Arduino framework)
**PlatformIO setting:** `board_build.partitions = default_8MB.csv`
**Target flash size:** 8 MB minimum — 16 MB recommended (N16R8 variant)

---

## Partition Table

| Name | Type | Sub-Type | Offset | Size (hex) | Size (human) | Role |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| *(Bootloader)* | — | — | `0x0000` | `0x8000` | 32 KB | Second-stage bootloader. Written by `esptool merge_bin` at offset `0x0`. Not listed in the CSV. |
| *(Partition table)* | — | — | `0x8000` | `0x1000` | 4 KB | Binary partition table itself. Offset is fixed by the ESP-IDF specification. |
| `nvs` | `data` | `nvs` | `0x9000` | `0x5000` | 20 KB | Non-Volatile Storage. Holds Wi-Fi SSID/password, AP/STA mode override flag, and any future NVS keys. Survives OTA and power cycles. |
| `otadata` | `data` | `ota` | `0xe000` | `0x2000` | 8 KB | OTA boot selector. Tracks which application partition (app0 or app1) is the active boot target. `boot_app0.bin` initialises this partition. |
| `app0` | `app` | `ota_0` | `0x10000` | `0x330000` | 3.19 MB | Primary application slot. The compiled firmware (`firmware.bin`) is flashed here. |
| `app1` | `app` | `ota_1` | `0x340000` | `0x330000` | 3.19 MB | OTA update slot. Reserved for future over-the-air firmware updates (v1.4.0 roadmap). Unused in v1.0.1. |
| `spiffs` | `data` | `spiffs` | `0x670000` | `0x180000` | 1.5 MB | LittleFS filesystem. Stores the web dashboard (`index.html`, `style.css`, `app.js`) and the dynamic point configuration (`/config.json`). Mounted as LittleFS via `board_build.filesystem = littlefs`. |
| `coredump` | `data` | `coredump` | `0x7F0000` | `0x10000` | 64 KB | Core dump storage. Captures a memory snapshot on firmware panic for post-mortem debugging. |

**Total:** 8 MB (`0x800000`)

---

## Key Addresses for `esptool merge_bin`

When building the merged release binary, these offsets are used explicitly:

```
0x0000   bootloader.bin
0x8000   partitions.bin
0xe000   boot_app0.bin        (OTA data — committed to docs/)
0x10000  firmware.bin         (application)
0x670000 littlefs.bin         (web dashboard + config)
```

See [BUILD_RELEASE.md](BUILD_RELEASE.md) for the full `merge_bin` command.

---

## Important Notes

### LittleFS vs SPIFFS
The partition sub-type is `spiffs` in the CSV (an ESP-IDF convention), but the firmware uses **LittleFS** via the `board_build.filesystem = littlefs` PlatformIO setting. LittleFS is flashed into this partition and provides better wear-levelling and power-loss resilience than the older SPIFFS driver.

### `config.json` persistence
The dynamic point configuration (`/config.json`) is stored in the LittleFS partition (`0x670000`). It is separate from the firmware partition and is **not overwritten** during a standard firmware OTA update — only a full filesystem re-flash replaces it.

### OTA readiness
The `app0` / `app1` dual-slot layout is already in place. OTA firmware update support (HTTP download + SHA-256 verification + rollback watchdog) is planned for **v1.4.0**.

### Changing the partition table
Any modification to `board_build.partitions` requires a full **Erase Flash** before re-uploading, or the device will boot with a stale partition table and may corrupt existing data.
