// Dashboard / base-station node: listens for float telemetry and displays it.
// Optionally sends a downlink command when the user button is pressed.
//
// Three hook points provided by lora_common.h:
//   on_setup_end  — extra initialisation after the radio is ready
//   on_loop       — called once per loop(); put any TX / UI logic here
//   on_rx         — called when a packet arrives with data/RSSI/SNR

#include "lora_common.h"

// ---------------------------------------------------------------------------
// Hook: called once at the end of common setup
// ---------------------------------------------------------------------------
void dash_setup_end() {
  both.println("Dashboard ready — listening...");
  // Add any display or UI init here
}

// ---------------------------------------------------------------------------
// Hook: called every loop()
// Send a downlink when the user button is pressed (button managed by heltec_loop).
// ---------------------------------------------------------------------------
void dash_loop() {
  if (button.isSingleClick()) {
    both.println("Sending ping...");
    lora_transmit("ping");
  }
}

// ---------------------------------------------------------------------------
// Hook: called when a LoRa packet has been received
// ---------------------------------------------------------------------------
void dash_rx(const String& data, float rssi, float snr) {
  both.printf("RX [%s]\r\n", data.c_str());
  both.printf("  RSSI: %.2f dBm\r\n", rssi);
  both.printf("  SNR:  %.2f dB\r\n",  snr);
  // TODO: parse telemetry fields, update display, log to SD card, etc.
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
  lora_set_callbacks(dash_setup_end, dash_loop, dash_rx);
  lora_setup();
}

void loop() {
  lora_loop();
}
