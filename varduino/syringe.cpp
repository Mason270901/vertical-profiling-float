#include "syringe.hpp"
#include <Arduino.h>

// ----------------------------------------------------------------------------
// Private state
// ----------------------------------------------------------------------------

static int s_pin_up   = -1;
static int s_pin_down = -1;

// microseconds per cc
// long factor = 127400; 
// long factor = 133770; // past 200
// long factor = 121030; // short of 50 short of 200
static long factor = 129948; // tiny bit past 200, dead on 50 first time, tiny bit below 50 on the return

// Accumulated microseconds: positive = water in, negative = water out.
// Only updated at motor transitions; never includes the in-progress segment.
static long delta_water = 0;

// Current motor direction: +1 = waterIn, -1 = waterOut, 0 = stopped
static int motor_state = 0;

// micros() timestamp when the current motion segment began
static unsigned long motion_start_us = 0;

// Target volume in cc
static int setpoint_cc = 0;

// ----------------------------------------------------------------------------
// Setup / reset
// ----------------------------------------------------------------------------

void syringeSetup(int pinUp, int pinDown) {
  s_pin_up   = pinUp;
  s_pin_down = pinDown;
  pinMode(s_pin_up,   OUTPUT);
  pinMode(s_pin_down, OUTPUT);
}

void syringeReset() {
  delta_water    = 0;
  motor_state    = 0;
  setpoint_cc    = 0;
  motion_start_us = 0;
}

// ----------------------------------------------------------------------------
// Low-level motor primitives
// ----------------------------------------------------------------------------

void waterOut() {
  digitalWrite(s_pin_up,   0);
  digitalWrite(s_pin_down, 1);
}

void waterIn() {
  digitalWrite(s_pin_up,   1);
  digitalWrite(s_pin_down, 0);
}

void waterStop() {
  digitalWrite(s_pin_up,   0);
  digitalWrite(s_pin_down, 0);
}

// -----------------------------------------------------------------------
// Commit the in-progress motion segment into delta_water, then apply
// the new motor state. micros() is called immediately before the motor
// call so the elapsed tally is as tight as possible.
// -----------------------------------------------------------------------
static void motorTransition(int new_state) {
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
    motion_start_us = now;
  }
}

// ----------------------------------------------------------------------------
// Public syringe API
// ----------------------------------------------------------------------------
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

// Fills buf with a JSON object: {"cc":N,"setpoint":N,"motor":"..."}
void syringeStatusStr(char *buf, int len) {
  int cc = syringeCC();
  const char *motor;
  if      (motor_state ==  1) motor = "running-in";
  else if (motor_state == -1) motor = "running-out";
  else                        motor = "stopped";
  snprintf(buf, len, "{\"cc\":%d,\"setpoint\":%d,\"motor\":\"%s\"}", cc, setpoint_cc, motor);
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
