#include "robot.h"
int motors[NUM_MOTORS];
int8_t throttle[NUM_MOTORS];

char message[3];

// minimal allocations
int8_t lin;
int8_t ang;
int8_t gun1;
int8_t gun2;

void setup() {
  // Sets all motors to respective pins
  configure_motors(motors);

  // Write initial frequency
  for (unsigned int i = 0U; i < NUM_MOTORS; ++i)
    analogWriteFrequency(motors[i], PWM_INIT);

  // Status led lights up and turns off everytime a message is recieved
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED1, HIGH);

  // Resolution to increase scopej
  analogWriteResolution(PWM_RES);

  // Serial1 is teensy's hardware serial (tx rx pin 0 and 1)
  Serial1.begin(BAUD);
  analogWriteResolution(12);
}

void loop() {

  // This reads data on the serial bus
  serial_listen(message, lin, ang, gun1, gun2);

  // Writes throttles to motors based on lin and ang
  callback_velocity(throttle, lin, ang, gun1, gun2);

  // Writes to motors (escs)
  motor_write(motors, throttle);

  // Milliseconds
  delay(10);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
}
