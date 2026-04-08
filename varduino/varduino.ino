#define MOTOR_UP 48
#define MOTOR_DOWN 47
// motor driver board is labeled
// in 1, in 2, in 3, in 4
// nc  , nc  , blue, orange

void waterOut() {
  // pushes water out
  digitalWrite(MOTOR_UP, 0);
  digitalWrite(MOTOR_DOWN, 1);
}

void waterIn() {
  // sucks water in
  digitalWrite(MOTOR_UP, 1);
  digitalWrite(MOTOR_DOWN, 0);
}

void waterStop() {
  digitalWrite(MOTOR_UP, 0);
  digitalWrite(MOTOR_DOWN, 0);
}

// used VCC of 10.5 bench supply

// microseconds per cc
// long factor = 127400; 
// long factor = 133770; // past 200
// long factor = 121030; // short of 50 short of 200
long factor = 129948; // tiny bit past 200, dead on 50 first time, tiny bit below 50 on the return

// Accumulated microseconds: positive = water in, negative = water out.
// Only updated at motor transitions; never includes the in-progress segment.
long delta_water = 0;

// Current motor direction: +1 = waterIn, -1 = waterOut, 0 = stopped
int motor_state = 0;

// micros() timestamp when the current motion segment began
unsigned long motion_start_us = 0;

// Target volume in cc
int setpoint_cc = 0;

// -----------------------------------------------------------------------
// Commit the in-progress motion segment into delta_water, then apply
// the new motor state. micros() is called immediately before the motor
// call so the elapsed tally is as tight as possible.
// -----------------------------------------------------------------------
void motorTransition(int new_state) {
  unsigned long now = micros();

  // Commit elapsed time from the current segment before changing state
  if (motor_state != 0) {
    long elapsed = (long)(now - motion_start_us);
    delta_water += elapsed * (long)motor_state;
  }

  // Drive motor — happens right after micros() call above
  if      (new_state ==  1) waterIn();
  else if (new_state == -1) waterOut();
  else                      waterStop();

  motor_state = new_state;
  if (new_state != 0) {
    motion_start_us = now; // start timing the new segment
  }
}

// -----------------------------------------------------------------------
// Syringe API
// -----------------------------------------------------------------------

// Returns instantaneous cc estimate, including any in-progress segment.
int syringeCC() {
  long current = delta_water;
  if (motor_state != 0) {
    unsigned long now = micros();
    long elapsed = (long)(now - motion_start_us);
    current += elapsed * (long)motor_state;
  }
  return (int)(current / factor);
}

// Set a new target volume in cc. syringeLoop() drives toward this value.
// Safe to call at any time regardless of current motor state or direction.
void syringeSetpoint(int cc) {
  setpoint_cc = cc;
}

// Returns motor state: +1 moving waterIn, -1 moving waterOut, 0 stopped.
int syringeState() {
  return motor_state;
}

// Prints current status to Serial.
void syringeStatus() {
  int cc = syringeCC();
  const char *state_str;
  if      (motor_state ==  1) state_str = "running-in";
  else if (motor_state == -1) state_str = "running-out";
  else                        state_str = "stopped";
  char buf[80];
  snprintf(buf, sizeof(buf), "cc:%d setpoint:%d state:%s", cc, setpoint_cc, state_str);
  Serial.println(buf);
}

// Call repeatedly from loop(). Drives motor toward setpoint_cc.
void syringeLoop() {
  int current = syringeCC();
  int error   = setpoint_cc - current;

  int desired;
  if      (error > 0) desired =  1;
  else if (error < 0) desired = -1;
  else                desired =  0;

  if (desired != motor_state) {
    motorTransition(desired);
  }
}

// -----------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_UP, OUTPUT);
  pinMode(MOTOR_DOWN, OUTPUT);

  // Empty the syringe at startup so position is known
  waterOut();
  delay(39000 - 3000);
  waterStop();

  // Syringe is empty; zero out all tracking state
  delta_water  = 0;
  motor_state  = 0;
  setpoint_cc  = 0;
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
