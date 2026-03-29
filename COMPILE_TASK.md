# Compile Task: SimpleWiFiServer for Heltec WiFi LoRa 32 V3

## Goal

Compile `SimpleWiFiServer/SimpleWiFiServer.ino` on Raspberry Pi (ARMv7) using `arduino-cli` targeting the Heltec WiFi LoRa 32 V3 (ESP32S3-based).

Source: https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/libraries/WiFi/examples/SimpleWiFiServer

```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 SimpleWiFiServer
```

---

## Findings & Issues Encountered

### 1. Wrong board core installed (`arduino:avr`)

Initial setup used `arduino:avr` (for ATmega). ESP32 requires `esp32:esp32`.

**Fix:**
```bash
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

---

### 2. Toolchain binary fails: `no such file or directory`

The esp32 core 3.x toolchain (`esp-x32`) ships soft-float ARM binaries expecting `/lib/ld-linux.so.3`, but Raspberry Pi OS only provides the hard-float linker `/lib/ld-linux-armhf.so.3`.

**Diagnosis:**
```
readelf: Flags: soft-float ABI
readelf: interpreter /lib/ld-linux.so.3
```

**Fix:** Install the soft-float cross libc and create the missing symlink:
```bash
sudo apt-get install -y libc6-armel-cross libgcc-s1-armel-cross
sudo ln -s /usr/arm-linux-gnueabi/lib/ld-linux.so.3 /lib/ld-linux.so.3
echo "/usr/arm-linux-gnueabi/lib" | sudo tee /etc/ld.so.conf.d/armel-cross.conf
sudo ldconfig
```

**Note:** A harmless warning appears for every compiled file due to `/etc/ld.so.preload` referencing the hard-float `libarmmem` — this can be ignored.

---

### 3. Missing library: `heltec_unofficial.h` (LoRa_rx_tx only — resolved, sketch removed)

The original LoRa_rx_tx sketch used `#include <heltec_unofficial.h>` from the unofficial library by [ropg](https://github.com/ropg/heltec_esp32_lora_v3).

**Fix (historical):**
```bash
arduino-cli config set library.enable_unsafe_install true

wget -q https://github.com/ropg/heltec_esp32_lora_v3/archive/refs/heads/main.zip \
  -O /tmp/heltec_unofficial.zip
arduino-cli lib install --zip-path /tmp/heltec_unofficial.zip
arduino-cli lib install "RadioLib"
arduino-cli lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays"
arduino-cli lib install "HotButton"
```

---

### 4. Linker errors — undefined references to `cleanupFunctional`, `EspClass` (LoRa_rx_tx — resolved)

`ropg/heltec_esp32_lora_v3` (v0.9.2) targets **esp32 core 2.x**. Core 3.x breaks API compatibility.

**Fix:** Downgrade to core 2.x:
```bash
arduino-cli core uninstall esp32:esp32   # frees ~5.5G (3.x + all tools)
arduino-cli core install esp32:esp32@2.0.17
```

---

### 5. Missing `libstdc++.so.6` for 2.x toolchain

The esp32 2.x toolchain (`xtensa-esp32s3-elf-gcc`) is also a soft-float ARM binary and additionally requires `libstdc++.so.6`.

**Fix:**
```bash
sudo apt-get install -y libstdc++6-armel-cross
sudo ldconfig
```

---

### 6. Sketch migration: LoRa_rx_tx → SimpleWiFiServer

`LoRa_rx_tx` was removed and replaced with the Heltec `SimpleWiFiServer` WiFi example.

The upstream example uses `NetworkServer`/`NetworkClient` (esp32 core 3.x API). Since the installed core is 2.0.17, the sketch was adapted to use `WiFiServer`/`WiFiClient` which are the equivalent 2.x API classes.

`WiFi.h` is bundled with the `esp32:esp32` core — no additional libraries were required.

---

## Final Working State ✓ CONFIRMED

**Compile output (EXIT:0):**
```
Sketch uses 682897 bytes (20%) of program storage space. Maximum is 3342336 bytes.
Global variables use 44176 bytes (13%) of dynamic memory, leaving 283504 bytes for local variables. Maximum is 327680 bytes.
```

**Compile succeeds with:**

| Component | Version |
|---|---|
| `esp32:esp32` core | 2.0.17 |
| `WiFi.h` | bundled with core (no separate install) |

**System packages required (soft-float ARM toolchain support):**
```bash
sudo apt-get install -y libc6-armel-cross libgcc-s1-armel-cross libstdc++6-armel-cross
sudo ln -s /usr/arm-linux-gnueabi/lib/ld-linux.so.3 /lib/ld-linux.so.3
echo "/usr/arm-linux-gnueabi/lib" | sudo tee /etc/ld.so.conf.d/armel-cross.conf
sudo ldconfig
```

**Full compile command:**
```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 SimpleWiFiServer
```

> Note: harmless `ERROR: ld.so: object '/usr/lib/arm-linux-gnueabihf/libarmmem-${PLATFORM}.so' from /etc/ld.so.preload cannot be preloaded` warnings appear on every compiled file — these are benign and can be ignored.
