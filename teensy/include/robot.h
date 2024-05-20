#ifndef ROBOT_H
#define ROBOT_H

#include "Arduino.h"

/**
 * I know this is lazy, but I'm lazy, also c++ is for dweeps, so
 * we want to keep memory allocation concentrated to compile time
 * macros are the best way to store data without memory allocation
 * this lets that smart ass compiler do its shit better
 */
// PWM Constants
#define PWM_SCALAR 5.12
#define PWM_ZERO 1535
#define PWM_RES 12
#define PWM_INIT 250

// Throttle and Robot Constants
#define MAX_THROTTLE 100
#define BAUD 9600
#define NUM_MOTORS 6

// Pins and indexes
#define LF_INDEX 0
#define LB_INDEX 1
#define RF_INDEX 2
#define RB_INDEX 3
#define GUN_1_IN 4
#define GUN_2_IN 5

#define LF_PIN 4
#define LB_PIN 5
#define RF_PIN 16
#define RB_PIN 7
#define GUN_1 29
#define GUN_2 30

#define LED1 15
#define LED2 18
#define LED3 21
int led1;
int led2;
int led3;

// Signals
#define DRIVE 'd'
#define STOP 's'

void led_toggle(int led, int pin) {
  if (led)
    digitalWrite(pin, HIGH);
  else
    digitalWrite(pin, LOW);
  led = !led;
}

/**
 * @brief Initialize motors
 * @note this is the most "changeable" thing - i.e. essentially this
 * is hard coded
 *
 * @param motors a motor array pointer
 */
void configure_motors(int motors[NUM_MOTORS]) {
  led1 = 1;
  led2 = 0;
  led3 = 0;
  motors[LF_INDEX] = LF_PIN;
  motors[LB_INDEX] = LB_PIN;
  motors[RB_INDEX] = RF_PIN;
  motors[RF_INDEX] = RB_PIN;
  motors[GUN_1_IN] = GUN_1;
  motors[GUN_2_IN] = GUN_2;
}

/**
 * @brief Convert a int8_t to a pwm likeable number //TODO change to macro
 *
 * @param throttle the dummed down throttle
 *
 * @return The nice PWM throttle
 */
int convert(const int8_t &throttle_) {
  int throttle;
  if (throttle_ < -MAX_THROTTLE)
    throttle = -MAX_THROTTLE;
  else if (throttle_ > MAX_THROTTLE)
    throttle = MAX_THROTTLE;
  else
    throttle = throttle_;
  return PWM_ZERO + (throttle * PWM_SCALAR);
}

/**
 * @brief Listens to the serial port and does stuff
 *
 * @param message A reference to a message so we are not allocating a ton of
 * data
 * @param lin A reference to a linear val so we are not allocating a ton of data
 * @param ang A reference to a ....
 */
int success; // No allocations
void serial_listen(char message[5], int8_t &lin, int8_t &ang, int8_t &gun1,
                   int8_t &gun2) {
  if (Serial1.available()) {
    success = Serial1.readBytes(message, 5) == 5;

    // here's our message format
    if (success && message[0] == DRIVE) {

      success = 1;
      lin = (message[1] == -1 ? (success = 0) : 2 * message[1]);
      ang = (message[2] == -1 ? (success = 0) : 2 * message[2]); // Temporary
      gun1 = (message[3] == -1 ? (success = 0) : 2 * message[3]);
      gun2 = (message[4] == -1 ? (success = 0) : 2 * message[4]);

      success ? digitalWrite(LED2, HIGH) : digitalWrite(LED3, HIGH);
    }

    // Stop stops the robot.
    else if (message[0] == STOP) {
      lin = 0;
      ang = 0;
    } else { // I know it's redundant, but I'm gonna change this later
      digitalWrite(LED3, HIGH); // Error
      lin = 0;
      ang = 0;
    }
  }
}

/**
 * @brief What did the chicken say to the dinosaur
 *
 * @param throttle I don't know
 * @param lin I didn't think this far ahead
 * @param ang Please don't get mad
 */
void callback_velocity(int8_t throttle[NUM_MOTORS], const int8_t &lin,
                       const int8_t &ang, const int8_t &gun1,
                       const int8_t gun2) {
  throttle[LF_INDEX] = -(lin + ang);
  throttle[LB_INDEX] = -(lin + ang);
  throttle[RF_INDEX] = lin - ang;
  throttle[RB_INDEX] = lin - ang;
  throttle[GUN_1_IN] = -gun1;
  throttle[GUN_2_IN] = gun2;
}

/**
 * @brief Write to dem motors
 *
 * @param motors[NUM_MOTORS] The global motor array
 * @param throttle[NUM_MOTORS] The global throttle array
 */
void motor_write(const int motors[NUM_MOTORS],
                 const int8_t throttle[NUM_MOTORS]) {
  for (int i = 0; i < NUM_MOTORS; i++)
    analogWrite(motors[i], convert(throttle[i]));
}

#endif
