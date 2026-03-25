# use 3 jobs to reduce the memory pressure

compile:
	arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --jobs 3 LoRa_rx_tx

program:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 LoRa_rx_tx