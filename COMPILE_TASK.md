# Compile Task: LoRa_rx_tx for Heltec WiFi LoRa 32 V3

## Goal

Compile `LoRa_rx_tx/LoRa_rx_tx.ino` on Raspberry Pi (ARMv7) using `arduino-cli` targeting the Heltec WiFi LoRa 32 V3 (ESP32S3-based).

```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 LoRa_rx_tx
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

### 3. Missing library: `heltec_unofficial.h`

The sketch uses `#include <heltec_unofficial.h>` from the unofficial library by [ropg](https://github.com/ropg/heltec_esp32_lora_v3). This is not in the arduino-cli registry.

**Fix:**
```bash
# enable_unsafe_install must be set first (edit arduino-cli.yaml or:)
arduino-cli config set library.enable_unsafe_install true

wget -q https://github.com/ropg/heltec_esp32_lora_v3/archive/refs/heads/main.zip \
  -O /tmp/heltec_unofficial.zip
arduino-cli lib install --zip-path /tmp/heltec_unofficial.zip
```

---

### 4. Missing dependency libraries (resolved one by one)

`heltec_unofficial.h` requires three additional libraries not auto-installed:

```bash
arduino-cli lib install "RadioLib"
arduino-cli lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays"
arduino-cli lib install "HotButton"
```

---

### 5. Linker errors — undefined references to `cleanupFunctional`, `EspClass`

After all libraries are installed, the link step fails:

```
undefined reference to `cleanupFunctional'
undefined reference to `ESP'
undefined reference to `_ZN8EspClass20getFlashFrequencyMHzEv'
```

**Root cause:** `ropg/heltec_esp32_lora_v3` library (v0.9.2) targets **esp32 core 2.x**. The installed core is **3.3.7**, which has breaking API changes in `EspClass` and C++ functional interrupt handling.

**Fix (attempted, failed due to disk space):** Downgrade to last stable 2.x core:
```bash
arduino-cli core install esp32:esp32@2.0.17
```
Failed with: `no space left on device` during toolchain extraction.

---

## Current Status

| Step | Status |
|---|---|
| arduino-cli installed | done |
| esp32:esp32 core installed (3.3.7) | done |
| Soft-float toolchain fix (`ld-linux.so.3`) | done |
| `heltec_unofficial` library installed | done |
| `RadioLib`, `SSD1306`, `HotButton` installed | done |
| Linker errors (core 2.x vs 3.x incompatibility) | **blocked** |
| Downgrade to `esp32:esp32@2.0.17` | **blocked — disk full** |

---

## Next Steps

1. **Free disk space** — the esp32 2.x toolchain needs ~500 MB to extract. Options:
   - Remove unused files: `sudo apt-get autoremove && sudo apt-get clean`
   - Remove the 3.x core if 2.x replaces it: `arduino-cli core uninstall esp32:esp32` before installing 2.x
   - Check: `df -h /` and `du -sh ~/.arduino15/`

2. **Install core 2.x:**
   ```bash
   arduino-cli core uninstall esp32:esp32
   arduino-cli core install esp32:esp32@2.0.17
   ```

3. **Retry compile:**
   ```bash
   arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 LoRa_rx_tx
   ```
