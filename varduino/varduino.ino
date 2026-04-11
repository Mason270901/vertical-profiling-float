

#include <Wire.h>
#include "MS5837.h"

// Bar30: MS5837_30BA (up to 300 m depth)
// Bar02: MS5837_02BA (up to 10 m depth)
// Change the model below if you have a Bar02.
#define SENSOR_MODEL MS5837::MS5837_30BA

// Pressure Sensor
// Red   - Vin   (5V)
// Green - SCL   (Clock)
// White - SDA   (Data)
// Black - GND

#define CLOCK 33
#define DATA  34

MS5837 sensor;

void setup() {
  Serial.begin(115200);
  Wire.begin(DATA, CLOCK);
  Wire.setClock(10000);

  // sensor.setModel(SENSOR_MODEL);

  while (!sensor.init()) {
    Serial.println("MS5837 init failed — check wiring");
    delay(1000);
  }

  // Freshwater density: 997 kg/m³  Saltwater: 1029 kg/m³
  sensor.setFluidDensity(997);
  // sensor.setFluidDensity(1029);

  Serial.println("MS5837 ready");
}

void loop() {
  sensor.read();

  char buf[120];
  snprintf(buf, sizeof(buf),
    "pressure=%.2f mbar  temp=%.2f C  depth=%.4f m  altitude=%.2f m",
    sensor.pressure(),
    sensor.temperature(),
    sensor.depth(),
    sensor.altitude());

  Serial.println(buf);

  delay(500);
}
