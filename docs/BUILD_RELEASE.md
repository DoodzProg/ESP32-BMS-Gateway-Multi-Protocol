# Build & Release Procedure

This document describes how to build the release binaries and publish a new version of the ESP32 BMS Gateway firmware.

> **Audience:** project maintainers and developers who want to produce the distributable binaries used by the GitHub Pages browser flash tool.

---

## Prerequisites

- PlatformIO CLI installed (`pip install platformio` or via VS Code extension).
- `esptool.py` available — bundled with PlatformIO: `~/.platformio/packages/tool-esptoolpy/esptool.py`.
- Python 3.8+.

---

## Step 1 — Build the Filesystem Image

The web dashboard assets in `data/` must be packaged into a LittleFS binary before building the firmware:

```bash
pio run --target buildfs
```

Output: `.pio/build/esp32-s3/littlefs.bin`

---

## Step 2 — Build the Firmware

```bash
pio run
```

This produces the following intermediate binaries in `.pio/build/esp32-s3/`:

| File | Role |
| :--- | :--- |
| `bootloader.bin` | Second-stage bootloader |
| `partitions.bin` | Compiled partition table |
| `firmware.bin` | Application firmware (app0 slot) |
| `littlefs.bin` | Web dashboard filesystem image |

---

## Step 3 — Merge All Binaries into a Single File

The browser flash tool (ESP Web Tools) requires a single merged binary that spans the full flash address space. Use `esptool.py merge_bin` to combine all four images at their correct offsets:

```bash
python -m esptool --chip esp32s3 merge_bin \
  --output docs/merged-firmware.bin \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 16MB \
  0x0000   .pio/build/esp32-s3/bootloader.bin \
  0x8000   .pio/build/esp32-s3/partitions.bin \
  0xe000   docs/boot_app0.bin \
  0x10000  .pio/build/esp32-s3/firmware.bin \
  0x670000 .pio/build/esp32-s3/littlefs.bin
```

> **Important:** `docs/boot_app0.bin` is the OTA data partition image. It is version-independent and committed to the repository. Do **not** replace it unless the partition table changes.

Output: `docs/merged-firmware.bin`

---

## Step 4 — Copy the Filesystem Image

Copy the LittleFS image into `docs/` so it can be flashed independently (useful for updating the web UI without reflashing the full firmware):

```bash
cp .pio/build/esp32-s3/littlefs.bin docs/littlefs.bin
```

---

## Step 5 — Update the Manifest

Edit `docs/manifest.json` to bump the version string before publishing:

```json
{
  "name": "ESP32 BMS Gateway",
  "version": "1.0.1",
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        { "path": "merged-firmware.bin", "offset": 0 }
      ]
    }
  ]
}
```

---

## Step 6 — Publish the GitHub Release

1. Tag the commit:
   ```bash
   git tag v1.0.1
   git push origin v1.0.1
   ```

2. Create the GitHub Release via the web UI or CLI:
   ```bash
   gh release create v1.0.1 \
     docs/merged-firmware.bin \
     docs/littlefs.bin \
     --title "v1.0.1 — Quality & Documentation" \
     --notes-file CHANGELOG.md
   ```

3. The GitHub Pages flash page at `https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol` automatically picks up the updated `docs/manifest.json` on the next push to `main`.

---

## Flash Address Reference

| Region | Offset | File |
| :--- | :---: | :--- |
| Bootloader | `0x0000` | `bootloader.bin` |
| Partition table | `0x8000` | `partitions.bin` |
| OTA data (boot_app0) | `0xe000` | `boot_app0.bin` |
| Application (app0) | `0x10000` | `firmware.bin` |
| LittleFS (web UI) | `0x670000` | `littlefs.bin` |

See [PARTITION_LAYOUT.md](PARTITION_LAYOUT.md) for the full partition table with sizes.

---

## Verifying the Merged Binary

After generating `merged-firmware.bin`, verify it is not corrupt:

```bash
python -m esptool image_info --version 2 docs/merged-firmware.bin
```

The output should list all segments at their expected offsets without errors.
