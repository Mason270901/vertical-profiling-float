#pragma once

// Turns the 'PRG' button into the power button, long press is off
// #define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

// Pause between transmitted packets in seconds.
// Set to zero to only transmit a packet when pressing the user button
// Will not exceed 1% duty cycle, even if you set a lower value.
#ifndef PAUSE
#define PAUSE               300
#endif

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
// #define FREQUENCY           866.3       // for Europe
#ifndef FREQUENCY
#define FREQUENCY           907.2       // for US
#endif

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#ifndef BANDWIDTH
#define BANDWIDTH           250.0
#endif

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference.
#ifndef SPREADING_FACTOR
#define SPREADING_FACTOR    9
#endif

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmitting without an antenna can damage your hardware.
#ifndef TRANSMIT_POWER
#define TRANSMIT_POWER      0
#endif

#define RADIOLIB2(action) \
  _radiolib_status = action;

#define RADIOLIB_OR_HALT2(action) \
  RADIOLIB2(action); \
  if (_radiolib_status != RADIOLIB_ERR_NONE) { \
    Serial.println("[RadioLib] Halted"); \
    while (true) { \
        RADIOLIB_DO_DURING_HALT; \
    } \
  }

// ---------------------------------------------------------------------------
// Callback types
// ---------------------------------------------------------------------------

// Called at the very end of setup(), after the radio is fully configured.
typedef void (*lora_void_fn)();

// Called once per loop() iteration, at the top of the loop — use this for
// TX logic or any periodic work.
typedef void (*lora_loop_fn)();

// Called when a LoRa packet has been received and decoded.
// data  : the received string payload
// rssi  : received signal strength in dBm
// snr   : signal-to-noise ratio in dB
typedef void (*lora_rx_fn)(const String& data, float rssi, float snr);

// ---------------------------------------------------------------------------
// Callback registrations — set these BEFORE calling lora_setup()
// ---------------------------------------------------------------------------
static lora_void_fn _on_setup_end = nullptr;
static lora_loop_fn _on_loop      = nullptr;
static lora_rx_fn   _on_rx        = nullptr;

inline void lora_set_callbacks(lora_void_fn on_setup_end,
                               lora_loop_fn on_loop,
                               lora_rx_fn   on_rx) {
  _on_setup_end = on_setup_end;
  _on_loop      = on_loop;
  _on_rx        = on_rx;
}

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static String   _rxdata;
static volatile bool _rxFlag = false;

// ISR — keep it minimal, no Serial/display calls allowed here
static void ARDUINO_ISR_ATTR _rx_isr() {
  _rxFlag = true;
}

// ---------------------------------------------------------------------------
// Common setup — call from your sketch's setup()
// ---------------------------------------------------------------------------
inline void lora_setup() {
  heltec_setup();
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());

  radio.setDio1Action(_rx_isr);

  both.printf("Frequency: %.2f MHz\r\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\r\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\r\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\r\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  if (_on_setup_end) _on_setup_end();
}

// ---------------------------------------------------------------------------
// Common loop — call from your sketch's loop()
// ---------------------------------------------------------------------------
inline void lora_loop() {
  heltec_loop();

  // Per-loop callback (TX logic, sensor reads, etc.)
  if (_on_loop) _on_loop();

  // RX handling
  if (_rxFlag) {
    _rxFlag = false;
    radio.readData(_rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      if (_on_rx) _on_rx(_rxdata, radio.getRSSI(), radio.getSNR());
    }
    RADIOLIB_OR_HALT2(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// ---------------------------------------------------------------------------
// Helper: transmit a string, temporarily disabling the RX interrupt.
// Returns true on success.
// ---------------------------------------------------------------------------
inline bool lora_transmit(const String& msg) {
  radio.clearDio1Action();
  heltec_led(50);
  uint64_t t = millis();
  RADIOLIB2(radio.transmit(msg.c_str()));
  uint64_t tx_time = millis() - t;
  heltec_led(0);

  bool ok = (_radiolib_status == RADIOLIB_ERR_NONE);
  if (ok) {
    both.printf("TX [%s] OK (%i ms)\r\n", msg.c_str(), (int)tx_time);
  } else {
    both.printf("TX [%s] fail (%i)\r\n", msg.c_str(), _radiolib_status);
  }

  radio.setDio1Action(_rx_isr);
  RADIOLIB_OR_HALT2(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  return ok;
}
