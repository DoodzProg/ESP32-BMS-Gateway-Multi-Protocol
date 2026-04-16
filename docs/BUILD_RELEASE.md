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

The browser flash tool (ESP Web Tools) requires a single merged binary that spans the full flash address space. Use `esptool.py merge_bin` to combine all four images at their correct offsets.

**Windows (PowerShell) — recommended:**

```powershell
# Resolve tool paths from the PlatformIO installation
$PYTHON   = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"
$ESPTOOL  = "$env:USERPROFILE\.platformio\packages\tool-esptoolpy\esptool.py"
$BOOT_APP = (Get-ChildItem "$env:USERPROFILE\.platformio\packages\framework-arduinoespressif32*" `
             -Recurse -Filter "boot_app0.bin" | Select-Object -First 1).FullName

& $PYTHON $ESPTOOL --chip esp32s3 merge_bin `
  --output "docs\merged-firmware.bin" `
  --flash_mode dio --flash_freq 80m --flash_size 8MB `
  0x0     ".pio\build\esp32-s3\bootloader.bin" `
  0x8000  ".pio\build\esp32-s3\partitions.bin" `
  0xe000  $BOOT_APP `
  0x10000 ".pio\build\esp32-s3\firmware.bin"
```

**Linux / macOS:**

```bash
BOOT_APP=$(find ~/.platformio/packages/framework-arduinoespressif32* \
           -name boot_app0.bin | head -1)

python ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32s3 merge_bin \
  --output docs/merged-firmware.bin \
  --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0     .pio/build/esp32-s3/bootloader.bin \
  0x8000  .pio/build/esp32-s3/partitions.bin \
  0xe000  "$BOOT_APP" \
  0x10000 .pio/build/esp32-s3/firmware.bin
```

> **Note:** `boot_app0.bin` is extracted directly from the PlatformIO Arduino framework package. It is version-independent and does not need to be committed to the repository.
>
> **Flash size:** this project uses `default_8MB.csv`; the `--flash_size` argument must match (`8MB`). The LittleFS partition at `0x670000` is flashed separately (Step 4) and is intentionally excluded from the merged binary so it can be updated independently.

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
  "name": "BMS Gateway",
  "version": "X.Y.Z",
  "new_install_prompt_erase": true,
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        { "path": "merged-firmware.bin", "offset": 0 },
        { "path": "littlefs.bin",        "offset": 6750208 }
      ]
    }
  ]
}
```

> The LittleFS offset `6750208` (`0x670000`) is fixed by `default_8MB.csv` and must not change between releases unless the partition table is modified.

---

## Step 6 — Publish the GitHub Release

1. Commit all changes, then create an annotated tag:
   ```powershell
   git add .
   git commit -m "release: vX.Y.Z — <short summary>"
   git tag -a vX.Y.Z -m "Release vX.Y.Z"
   git push origin main
   git push origin vX.Y.Z
   ```

2. Create the GitHub Release via the web UI (recommended) or CLI:
   ```bash
   gh release create vX.Y.Z \
     docs/merged-firmware.bin \
     docs/littlefs.bin \
     --title "vX.Y.Z — <release title>" \
     --notes-file CHANGELOG.md
   ```

3. The GitHub Pages flash page at `https://DoodzProg.github.io/ESP32-BMS-Gateway-Multi-Protocol` automatically picks up the updated `docs/manifest.json` on the next push to `main`.

> **Annotated tags** (`-a`) are required for GitHub to correctly render the release in the tags list and for `git describe` to work. Lightweight tags (`git tag vX.Y.Z`) are not sufficient for release publishing.

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
