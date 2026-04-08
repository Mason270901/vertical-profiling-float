#define MOTOR_UP 48
#define MOTOR_DOWN 47
// motor driver board is labeled
// in 1, in 2, in 3, in 4
// nc  , nc  , blue, orange

// #define MOTOR_FEEDBACK 4
// #define HALL_TEST 3



// only read while interrupts are disabled
// volatile long isr_pulse_counts = 0;
// volatile bool isr_up = false;

// volatile unsigned long last_toggle_us = 0;

// don't print or do any delays in this isr
// void ISR_pulse() {
//   isr_pulse_counts++;

  
//   unsigned long now = micros();
//   // Serial.print("now: ");
//   // Serial.print(now);
//   // Serial.print(" lst: ");
//   // Serial.print(last_toggle_us);
//   // Serial.print(" d: ");
//   // Serial.println( (now - last_toggle_us) );

//   if (now - last_toggle_us > 2000) {  // 2ms debounce
//     // if( 1) {
//     last_toggle_us = now;
//     isr_up = !isr_up;
//     digitalWrite(HALL_TEST, isr_up);
//   }
// }

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


void setup() {
  Serial.begin(115200);
  // setup motor
  pinMode(MOTOR_UP, OUTPUT);
  pinMode(MOTOR_DOWN, OUTPUT);

  // test output to see how hall effect is working
  // pinMode(HALL_TEST, OUTPUT);

  // setup feedback
  // pinMode(MOTOR_FEEDBACK, INPUT);

  // attachInterrupt(digitalPinToInterrupt(MOTOR_FEEDBACK), ISR_pulse, RISING);


  // analogWrite(MOTOR_UP, 190);

  waterOut();
  delay(39000-4000);

  // // takes water in
  // digitalWrite(MOTOR_UP, 1);
  // digitalWrite(MOTOR_DOWN, 0);
  // delay(39000);

  // digitalWrite(MOTOR_UP, 0);
  // digitalWrite(MOTOR_DOWN, 0);
  waterStop();
}

#define RUN

// positive means we were drawing water
long delta_water;

// how many us per ml
// long factor = 130000;
long factor = 127400;

int waterStatus() {
  return delta_water / factor;
}

void loop() {

  long count;

  #ifdef RUN
  // // takes water in
  // digitalWrite(MOTOR_UP, 1);
  // digitalWrite(MOTOR_DOWN, 0);

  // take micros, then call fn immediatly
  unsigned long now = micros();
  waterIn();

  for(int i = 0; i < 6; i++) {
    Serial.print("i: ");
    Serial.println(i);
    delay(1000);
  }
  delay(500);

  unsigned long then = micros();
  waterStop();

  unsigned long delta = then-now;

  delta_water += delta;

  char buf[128];
  snprintf(buf, sizeof(buf), "delta: %lu tot delta: %ld est: %d", delta, delta_water, waterStatus());
  Serial.println(buf);

  

  delay(4000);
    // Serial.print(" counts: ");
    // Serial.println(count);


  // for(int i = 0; i < 39; i++) {

  //   // delay(39000);
    
  //   // turn off interrupts
  //   // copy out the value
  //   // turn them back on
  //   noInterrupts();
  //   count = isr_pulse_counts;
  //   interrupts();

  //   Serial.print("i: ");
  //   Serial.print(i);
  //   Serial.print(" counts: ");
  //   Serial.println(count);

  //   delay(1000);
  // }

  // digitalWrite(MOTOR_UP, 0);
  // digitalWrite(MOTOR_DOWN, 1);

  // for(int i = 0; i < 39; i++) {

  //   // delay(39000);
    
  //   // turn off interrupts
  //   // copy out the value
  //   // turn them back on
  //   noInterrupts();
  //   count = isr_pulse_counts;
  //   interrupts();

  //   Serial.print("i: ");
  //   Serial.print(i);
  //   Serial.print(" counts: ");
  //   Serial.println(count);

  //   delay(1000);
  // }


#else
    digitalWrite(MOTOR_UP, 0);
    digitalWrite(MOTOR_DOWN, 0);
#endif




}
