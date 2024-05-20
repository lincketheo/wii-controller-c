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
#define NUM_MOTORS 4

// Pins and indexes
#define LF_INDEX 0
#define LB_INDEX 1
#define RF_INDEX 2
#define RB_INDEX 3

#define LF_PIN 4
#define LB_PIN 5
#define RF_PIN 6
#define RB_PIN 7
#define STATUSLED 8

// Signals
#define DRIVE 'd'
#define STOP 's'

int status_led;

/**
 * @brief Initialize motors
 * @note this is the most "changeable" thing - i.e. essentially this
 * is hard coded
 *
 * @param motors a motor array pointer
 */
void configure_motors(int motors[NUM_MOTORS]) {
  status_led = 0;
  motors[LF_INDEX] = LF_PIN;
  motors[LB_INDEX] = LB_PIN;
  motors[RB_INDEX] = RF_PIN;
  motors[RF_INDEX] = RB_PIN;
}

/**
 * @brief Convert a int8_t to a pwm likeable number
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
void serial_listen(char message[3], int8_t &lin, int8_t &ang) {
  if (Serial1.available()) {
    // set the status led
    digitalWrite(STATUSLED, status_led);
    status_led = !status_led;
    message[0] = Serial1.read();

    // here's our message format
    if (message[0] == DRIVE) {
      message[1] = Serial1.read();
      message[2] = Serial1.read();

      // Yay C++ hidden in c
      lin = (message[1] == -1 ? 0 : 10 * message[1]);
      ang = (message[2] == -1 ? 0 : 10 * message[2]); // Temporary
    }

    // Stop stops the robot.
    else if (message[0] == STOP) {
      lin = 0;
      ang = 0;
    } else { // I know it's redundant, but I'm gonna change this later
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
                       const int8_t &ang) {
  throttle[LF_INDEX] = -(lin + ang);
  throttle[LB_INDEX] = -(lin + ang);
  throttle[RF_INDEX] = lin - ang;
  throttle[RF_INDEX] = lin - ang;
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
