IMAGE := vp-float-compiler:1
TARGET_PI := vert

# Build the x86 Docker image that contains arduino-cli + all required cores/libraries
docker-build:
	$(MAKE) -C docker all

# Compile inside Docker — output lands in LoRa_rx_tx/build/ on the host via the volume mount
compile-docker:
	docker run --rm -v $(PWD):/src $(IMAGE) \
		arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --jobs 3 \
		--output-dir /src/LoRa_rx_tx/build LoRa_rx_tx

# Push the build folder to the Pi (replaces the remote build/ dir entirely)
push:
	rsync -av --delete LoRa_rx_tx/build/ $(TARGET_PI):~/vertical-profiling-float/LoRa_rx_tx/build/

# --- targets below require arduino-cli installed directly on the host ---

# use 3 jobs to reduce the memory pressure
compile:
	arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --jobs 3 LoRa_rx_tx

program:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 LoRa_rx_tx

program-build:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 \
		--input-dir LoRa_rx_tx/build LoRa_rx_tx