#pragma once

// Call once from setup(). Stores pin numbers and calls pinMode.
void syringeSetup(int pinUp, int pinDown);

// Zero all tracking state (delta_water, motor_state, setpoint_cc).
// Call after a manual empty/fill sequence so syringeLoop() starts clean.
void syringeReset();

// Low-level motor primitives — exposed so setup() can drive the motor
// directly without going through the setpoint machinery.
void waterOut();   // pushes water out
void waterIn();    // sucks water in
void waterStop();

// Returns instantaneous volume estimate in cc (includes in-progress segment).
int  syringeCC();

// Set a new target volume in cc. syringeLoop() drives toward this value.
void syringeSetpoint(int cc);

// Returns motor state: +1 moving waterIn, -1 moving waterOut, 0 stopped.
int  syringeState();

// Prints current status to Serial: cc, setpoint, and motor direction.
void syringeStatus();

// Call repeatedly from loop(). Drives motor toward setpoint_cc.
void syringeLoop();
