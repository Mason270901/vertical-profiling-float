#include "syringe.hpp"

#define MOTOR_UP   48
#define MOTOR_DOWN 47
// motor driver board is labeled
// in 1, in 2, in 3, in 4
// nc  , nc  , blue, orange

// used VCC of 10.5 bench supply

// -----------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  syringeSetup(MOTOR_UP, MOTOR_DOWN);

  // Empty the syringe at startup so position is known
  waterOut();
  delay(39000 - 1000);
  waterStop();

  delay(4000);

  waterIn();
  delay(39000 - 1000);
  waterStop();

  // Zero all tracking state — syringe is now at a known empty position
  syringeReset();
}

void loop() {
  char buf[80];
  syringeLoop();

  for(int i = 0; i < 1000; i++) {

    syringeLoop();

    if (i == 2) {
      syringeSetpoint(50);
      snprintf(buf, sizeof(buf), "asking for 50");
      Serial.println(buf);
    } else if (i == 200) {
      syringeSetpoint(200);
      snprintf(buf, sizeof(buf), "asking for 200");
      Serial.println(buf);
    } else if (i == 600) {
      syringeSetpoint(50);
      snprintf(buf, sizeof(buf), "asking for 50");
      Serial.println(buf);
    } else {
      // digitalWrite(MOTOR_UP, 0);
      // digitalWrite(MOTOR_DOWN, 0);
    }

    

    syringeLoop();

    delay(100);

    syringeLoop();

  // Print status once per second
    static unsigned long last_print_ms = 0;
    unsigned long now_ms = millis();
    if (now_ms - last_print_ms >= 400) {
      last_print_ms = now_ms;
      syringeStatus();

      snprintf(buf, sizeof(buf), "i: %d", i);
      Serial.println(buf);
    }

  }

 
}
