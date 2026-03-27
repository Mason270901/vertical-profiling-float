FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LC_ALL=C

RUN apt-get update && apt-get install -y \
    curl wget ca-certificates \
    python3 python3-serial \
    && rm -rf /var/lib/apt/lists/*

# Install arduino-cli (latest)
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
    | BINDIR=/usr/local/bin sh

# Bootstrap config, add esp32 board manager URL, allow zip installs
RUN arduino-cli config init && \
    arduino-cli config add board_manager.additional_urls \
        https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json && \
    arduino-cli config set library.enable_unsafe_install true

# Install esp32 core — pinned to 2.0.17.
# 3.x has breaking API changes that break the heltec_unofficial library (v0.9.2).
RUN arduino-cli core update-index && \
    arduino-cli core install esp32:esp32@2.0.17

# Install heltec_unofficial library (not in the arduino registry, must use zip)
RUN wget -q https://github.com/ropg/heltec_esp32_lora_v3/archive/refs/heads/main.zip \
        -O /tmp/heltec_unofficial.zip && \
    arduino-cli lib install --zip-path /tmp/heltec_unofficial.zip && \
    rm /tmp/heltec_unofficial.zip

# Install dependency libraries (pinned to known-good versions)
RUN arduino-cli lib install "RadioLib@7.6.0"
RUN arduino-cli lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays@4.6.2"
RUN arduino-cli lib install "HotButton@0.1.1"

WORKDIR /src
