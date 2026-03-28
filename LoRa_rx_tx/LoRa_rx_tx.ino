

// Turns the 'PRG' button into the power button, long press is off 
// #define HELTEC_POWER_BUTTON   // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

// Pause between transmited packets in seconds.
// Set to zero to only transmit a packet when pressing the user button
// Will not exceed 1% duty cycle, even if you set a lower value.
#define PAUSE               300

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
// #define FREQUENCY           866.3       // for Europe
#define FREQUENCY           907.2       // for US

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH           250.0

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference. 
#define SPREADING_FACTOR    9

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmissting without an antenna can damage your hardware.
#define TRANSMIT_POWER      0

#define RADIOLIB2(action) \
  _radiolib_status = action;
  // Serial.print("[RadioLib] "); \
  // Serial.print(#action); \
  // Serial.print(" returned "); \
  // Serial.print(_radiolib_status); \
  // Serial.print(" ("); \
  // Serial.print(radiolib_result_string(_radiolib_status)); \
  // Serial.println(")");

#define RADIOLIB_OR_HALT2(action) \
  RADIOLIB2(action); \
  if (_radiolib_status != RADIOLIB_ERR_NONE) { \
    Serial.println("[RadioLib] Halted"); \
    while (true) { \
        RADIOLIB_DO_DURING_HALT; \
    } \
  }

String rxdata;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;

void setup() {
  heltec_setup();
  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\r\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\r\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\r\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\r\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  // Serial.println("Loop start");
  heltec_loop();
  // Serial.println("Loop end");
  
  // Transmit a packet every PAUSE seconds or when the button is pressed
  if ( (millis() - last_tx > (PAUSE)) ) {
    both.printf("Vert TX [%s] ", String(counter).c_str());
    radio.clearDio1Action();
    heltec_led(50); // 50% brightness is plenty for this LED
    tx_time = millis();
    RADIOLIB2(radio.transmit(String(counter++).c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\r\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\r\n", _radiolib_status);
    }
    last_tx = millis();
    radio.setDio1Action(rx);
    RADIOLIB_OR_HALT2(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  } else {
    // 
  }

  // If a packet was received, display it and the RSSI and SNR
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\r\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\r\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\r\n", radio.getSNR());
    }
    RADIOLIB_OR_HALT2(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// Can't do Serial or display things here, takes too much time for the interrupt
void rx() {
  rxFlag = true;
}
