IMAGE := vp-float-compiler:1
TARGET_PI := vert
# When on the PI 2, fewer jobs is safer for memory pressure
# ARDUINO_JOBS ?= --jobs 3
ARDUINO_JOBS ?=

all: compile

# Build the x86 Docker image that contains arduino-cli + all required cores/libraries
docker-build:
	$(MAKE) -C docker all

# Compile inside Docker — output lands in WiFiAccessPoint/build/ on the host via the volume mount
compile-docker:
	docker run --rm -v $(PWD):/src $(IMAGE) \
		arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 $(ARDUINO_JOBS) \
		--output-dir /src/WiFiAccessPoint/build WiFiAccessPoint

# Push the build folder to the Pi (replaces the remote build/ dir entirely)
push:
	rsync -av --delete WiFiAccessPoint/build/ $(TARGET_PI):~/vertical-profiling-float/WiFiAccessPoint/build/

# --- targets below require arduino-cli installed directly on the host ---

# use 3 jobs to reduce the memory pressure
compile:
	arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 $(ARDUINO_JOBS) WiFiAccessPoint

program:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 WiFiAccessPoint

program-build:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 \
		--input-dir WiFiAccessPoint/build WiFiAccessPoint
