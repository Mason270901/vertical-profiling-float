IMAGE := vp-float-compiler:1
TARGET_PI := vert
# When on the PI 2, fewer jobs is safer for memory pressure
# ARDUINO_JOBS ?= --jobs 3
ARDUINO_JOBS ?=
FQBN := esp32:esp32:heltec_wifi_lora_32_V3
PORT ?= /dev/ttyUSB0
COMMON_DIR := $(abspath arduino/common)
EXTRA_FLAGS := --build-property "compiler.cpp.extra_flags=-I$(COMMON_DIR)" --build-property "compiler.c.extra_flags=-I$(COMMON_DIR)"

all: compile-vert compile-vdashboard

# Build the x86 Docker image that contains arduino-cli + all required cores/libraries
docker-build:
	$(MAKE) -C docker all

# Compile inside Docker — output lands in the sketch's build/ dir on the host via volume mount
compile-docker-vert:
	docker run --rm -v $(PWD):/src $(IMAGE) \
		arduino-cli compile --fqbn $(FQBN) $(ARDUINO_JOBS) \
		--build-property "compiler.cpp.extra_flags=-I/src/arduino/common" \
		--build-property "compiler.c.extra_flags=-I/src/arduino/common" \
		--output-dir /src/arduino/vert/build arduino/vert

compile-docker-vdashboard:
	docker run --rm -v $(PWD):/src $(IMAGE) \
		arduino-cli compile --fqbn $(FQBN) $(ARDUINO_JOBS) \
		--build-property "compiler.cpp.extra_flags=-I/src/arduino/common" \
		--build-property "compiler.c.extra_flags=-I/src/arduino/common" \
		--output-dir /src/arduino/vdashboard/build arduino/vdashboard

# Push compiled vert firmware to the Pi
push:
	rsync -av --delete arduino/vert/build/ $(TARGET_PI):~/vertical-profiling-float/arduino/vert/build/

# --- targets below require arduino-cli installed directly on the host ---

compile-vert:
	arduino-cli compile --fqbn $(FQBN) $(ARDUINO_JOBS) $(EXTRA_FLAGS) arduino/vert

compile-vdashboard:
	arduino-cli compile --fqbn $(FQBN) $(ARDUINO_JOBS) $(EXTRA_FLAGS) arduino/vdashboard

program-vert:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) arduino/vert

program-vdashboard:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) arduino/vdashboard

# Legacy targets (original monolithic sketch)
compile-legacy:
	arduino-cli compile --fqbn $(FQBN) $(ARDUINO_JOBS) LoRa_rx_tx

program-legacy:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) LoRa_rx_tx

program-build-legacy:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) \
		--input-dir LoRa_rx_tx/build LoRa_rx_tx
