# vertical-profiling-float

# Operation

```bash
make && make program && screen -L -Logfile "logs/$(date +%Y%m%d_%H%M%S)_session-log.txt" /dev/ttyUSB0 115200
```

# RPI programmer
If using an RPI to program, use a 32G SD card, the 16G might work but it gets very tight.

# RPI Details
Below, I started with a RPI 2, and 2025-12-04-raspios-trixie-armhf.img


# Board

* Board: https://heltec.org/project/wifi-lora-32-v3/

# Arduino IDE support (NOT WORKING as of now)
* https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html#via-arduino-ide
* board manager url is `https://resource.heltec.cn/download/package_heltec_esp32_index.json`
* click board manager
* installed Heltec ESP32 Series Dev-boards 3.0.3
  * enter "heltec esp32" in the search box
* click library manager
  * "HELTEC ESP32", select the latest version and install
  * after that, with the same search still in, get Heltec_ESP32_LoRa_v3

# Arduino CLI setup (Raspberry Pi as programmer)

## 1. Install arduino-cli

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/
```

## 2. Fix toolchain shared library dependencies

The ESP32 toolchain ships soft-float ARM binaries, but Raspberry Pi OS only has the
hard-float linker. Install the soft-float cross libs and wire them up:

```bash
sudo apt-get install -y libc6-armel-cross libgcc-s1-armel-cross libstdc++6-armel-cross
sudo ln -s /usr/arm-linux-gnueabi/lib/ld-linux.so.3 /lib/ld-linux.so.3
echo "/usr/arm-linux-gnueabi/lib" | sudo tee /etc/ld.so.conf.d/armel-cross.conf
sudo ldconfig
```

> After this you will see harmless `ld.so: object '/usr/lib/arm-linux-gnueabihf/libarmmem-${PLATFORM}.so' cannot be preloaded` warnings during compile — ignore them.

## 3. Install esp32 core (pinned to 2.0.17)

Must use **2.0.17** — the `heltec_unofficial` library is incompatible with core 3.x.
The core is ~2GB and takes ~20 mins to download/install on the Pi.
A 15G SD card will be at ~64% after install (leaves ~5G free).

```bash
arduino-cli config init

arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
```

## 4. Install libraries

`heltec_unofficial.h` is not in the arduino-cli registry — install from GitHub zip:

```bash
arduino-cli config set library.enable_unsafe_install true

wget -q https://github.com/ropg/heltec_esp32_lora_v3/archive/refs/heads/main.zip \
  -O /tmp/heltec_unofficial.zip
arduino-cli lib install --zip-path /tmp/heltec_unofficial.zip
```

Install the three required dependencies:

```bash
arduino-cli lib install "RadioLib"
arduino-cli lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays"
arduino-cli lib install "HotButton"
```

## 5. Compile

```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 LoRa_rx_tx
```

## Find the exact FQBN for your hardware revision with:

```bash
arduino-cli board listall heltec
```
