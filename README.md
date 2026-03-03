# WLED-Bambu 🖨️💡

> WLED firmware with native **Bambu Lab P1S** printer status LEDs.  
> No Home Assistant, no Node-RED — just your ESP32 and your printer.

---

## Features

- Polls your P1S over HTTP every 2 seconds
- 6 configurable states: **printing · heating · cooling · idle · downloading · error**
- Full WLED effect library (114 effects) per state — set via browser UI
- Color pickers, speed, intensity per state
- Persists config to flash (SPIFFS)
- Built on stock WLED 0.14 — all standard WLED features still work

---

## Getting the .bin (GitHub Actions — no local toolchain needed)

1. **Fork this repo** on GitHub
2. Go to **Actions** tab → enable workflows if prompted
3. Push any commit (or click **Run workflow** manually)
4. Download `WLED-Bambu-firmware` from the **Artifacts** section of the build
5. Flash `firmware.bin` to your ESP32

---

## Flashing

### Easiest — ESPHome Flasher (GUI)
Download from https://github.com/esphome/esphome-flasher/releases  
Select your COM port, select `firmware.bin`, click Flash.

### esptool.py (command line)
```bash
pip install esptool
esptool.py --chip esp32 --port /dev/ttyUSB0 \
  write_flash 0x10000 firmware.bin
```

### Full erase first (recommended for first flash)
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
esptool.py --chip esp32 --port /dev/ttyUSB0 \
  write_flash 0x10000 firmware.bin
```

---

## First boot setup

1. Connect to WiFi hotspot **`WLED-AP`** (password: `wled1234`)
2. Browser opens automatically → enter your home WiFi credentials
3. Navigate to `http://<device-ip>/bambu`
4. Enter your **printer IP address** and tick **Enabled**
5. Set your preferred effect/color for each printer state
6. Click **Save & Apply**

---

## Wiring

| ESP32 Pin | Wire to        |
|-----------|----------------|
| GPIO 5    | LED strip DATA |
| GND       | LED strip GND  |
| 5V / VIN  | LED strip +5V  |

> LED pin can be changed in `platformio.ini` → `LEDPIN=5`

---

## Project structure

```
wled-bambu/
├── .github/workflows/build.yml   ← GitHub Actions CI
├── platformio.ini
├── tools/
│   └── patch_wled.py             ← auto-patches wled00.ino
├── src/
│   ├── bambu_status.h
│   ├── bambu_status.cpp            ← HTTP poll + effect apply
│   ├── wled_bambu_server.cpp    ← /bambu/status and /bambu/config routes
│   └── wled00.ino                ← integration hooks
└── data/
    ├── bambu.htm                   ← browser effect editor
    └── bambu_default.json          ← default effects
```

---

## Printer states mapped

| State        | Trigger                              | Default effect       |
|--------------|--------------------------------------|----------------------|
| `printing`   | `gcode_state = RUNNING`              | White breathe        |
| `heating`    | `gcode_state = PREPARE` or hot nozzle| Orange pulse         |
| `cooling`    | Post-print cooling                   | Blue breathe         |
| `idle`       | `gcode_state = IDLE`                 | Warm white solid     |
| `downloading`| `gcode_state = downloading`          | Cyan running         |
| `error`      | `gcode_state = FAILED/ERROR`         | Red flash            |

---

## Building locally

```bash
git clone https://github.com/Aircoookie/WLED.git --branch 0.14.0
# Copy src/* → WLED/wled00/
# Copy data/* → WLED/data/
# Copy platformio.ini → WLED/
python3 tools/patch_wled.py WLED/wled00/wled00.ino
cd WLED
pio run -e esp32dev
# Binary at: .pio/build/esp32dev/firmware.bin
```
