// Vertical-float node: periodically transmits a counter/sensor packet
// and logs any received downlink messages.
//
// Three hook points provided by lora_common.h:
//   on_setup_end  — extra initialisation after the radio is ready
//   on_loop       — called once per loop(); put TX logic here
//   on_rx         — called when a packet arrives with data/RSSI/SNR

#include "lora_common.h"

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
static long     _counter  = 0;
static uint64_t _last_tx  = 0;

// ---------------------------------------------------------------------------
// Hook: called once at the end of common setup
// ---------------------------------------------------------------------------
void vert_setup_end() {
  both.println("Vert node ready");
  // Add any sensor or peripheral init here
}

// ---------------------------------------------------------------------------
// Hook: called every loop() — handle periodic TX
// ---------------------------------------------------------------------------
void vert_loop() {
  if (millis() - _last_tx > PAUSE) {
    String msg = String(_counter++);
    both.printf("Vert TX [%s] ", msg.c_str());
    lora_transmit(msg);
    _last_tx = millis();
  }
}

// ---------------------------------------------------------------------------
// Hook: called when a LoRa packet has been received
// ---------------------------------------------------------------------------
void vert_rx(const String& data, float rssi, float snr) {
  both.printf("RX [%s]\r\n", data.c_str());
  both.printf("  RSSI: %.2f dBm\r\n", rssi);
  both.printf("  SNR:  %.2f dB\r\n",  snr);
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
  lora_set_callbacks(vert_setup_end, vert_loop, vert_rx);
  lora_setup();
}

void loop() {
  lora_loop();
}
