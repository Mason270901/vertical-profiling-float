IMAGE := vp-float-compiler:1
TARGET_PI := vert
# When on the PI 2, fewer jobs is safer for memory pressure
# ARDUINO_JOBS ?= --jobs 3
ARDUINO_JOBS ?=

# --- LittleFS filesystem image tooling ---
MKLITTLEFS := $(HOME)/.arduino15/packages/esp32/tools/mklittlefs/3.0.0-gnu12-dc7f933/mklittlefs
ESPTOOL    := python3 $(HOME)/.arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool.py
FS_PORT    ?= /dev/ttyUSB0
# Partition table: default_8MB.csv — spiffs offset=0x670000, size=0x180000 (1572864 bytes)
FS_OFFSET  := 0x670000
FS_SIZE    := 1572864
FS_DATA    := varduino/data
FS_IMAGE   := varduino/littlefs.bin

all: compile

# Build the x86 Docker image that contains arduino-cli + all required cores/libraries
docker-build:
	$(MAKE) -C docker all

# Compile inside Docker — output lands in varduino/build/ on the host via the volume mount
compile-docker:
	docker run --rm -v $(PWD):/src $(IMAGE) \
		arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 $(ARDUINO_JOBS) \
		--output-dir /src/varduino/build varduino

# Push the build folder to the Pi (replaces the remote build/ dir entirely)
push:
	rsync -av --delete varduino/build/ $(TARGET_PI):~/vertical-profiling-float/varduino/build/

# --- targets below require arduino-cli installed directly on the host ---

# use 3 jobs to reduce the memory pressure
compile:
	arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 $(ARDUINO_JOBS) varduino

program:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 varduino

program-build:
	arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 \
		--input-dir varduino/build varduino

# Build a LittleFS image from WiFiAccessPoint/data/ on the host
# Usage: make fs-build
build-fs:
	$(MKLITTLEFS) -c $(FS_DATA) -s $(FS_SIZE) -b 4096 -p 256 $(FS_IMAGE)

clean:
	rm -f $(FS_IMAGE)

clean-cache:
	arduino-cli cache clean

# Flash the LittleFS image to the spiffs partition (offset 0x670000)
# Usage: make fs-flash [FS_PORT=/dev/ttyUSB0]
program-data: build-fs
	$(ESPTOOL) --chip esp32s3 --port $(FS_PORT) --baud 921600 \
		write_flash $(FS_OFFSET) $(FS_IMAGE)
