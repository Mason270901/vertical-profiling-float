# vertical-profiling-float

* Board: https://heltec.org/project/wifi-lora-32-v3/

# Arduino IDE support
* https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html#via-arduino-ide
* board manager url is `https://resource.heltec.cn/download/package_heltec_esp32_index.json`
* click board manager
* installed Heltec ESP32 Series Dev-boards 3.0.3
  * enter "heltec esp32" in the search box
* click library manager
  * "HELTEC ESP32", select the latest version and install
  * after that, with the same search still in, get Heltec_ESP32_LoRa_v3

# Arduino CLI setup (Raspberry Pi → ESP32)

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/

arduino-cli config init

# Add ESP32 board manager URL before updating index
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  
arduino-cli core update-index

# this takes like 20 mins
arduino-cli core install esp32:esp32

# Install the Heltec unofficial library used by the sketch
arduino-cli lib install "Heltec ESP32 Dev-Boards"
```

To compile, specify the Heltec board FQBN:

```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 LoRa_rx_tx
```

Find the exact FQBN for your hardware revision with:

```bash
arduino-cli board listall heltec
```
