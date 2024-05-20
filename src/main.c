
#include "utils.h"
#include <signal.h>

/**
 * These are globals
 * Globals are bad.
 * Please don't use globals.
 */
void init_state_system() {

  // Set signal to sigint and term
  // int is ctrl c
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  scan_signal = 0;
  running = 1;

  wii_ready = 0;
  serial_ready = 0;

  // Create the main robot
  robot_main = kermit_robot();
}

int main() {

  init_state_system();

  /**
   * You can set robot options. Valid options are:
   *
   * VAR_SPEED: Makes the robot increase in speed, if false, the robot either
   * drives or doesnt VERBOSE: Prints output to the screen INSYNC: Keeps the wii
   * controller thread at the same rate as the serial thread NONLIN: Non linear
   * increase in speed, does nothing if var speed is false DISCLINANG: Eitehr
   * turns or drives. Can't do both DEBUG: ignores serial port all together
   * ADVNCD: If you press the + button on the remote, you enter advanced, where
   * you can change any of the above options except debug.
   *
   * NOTE defaults to 00000000
   */
  robot_unsetopt(&robot_main, DEBUG);
  robot_setopt(&robot_main, VERBOSE);
  robot_unsetopt(&robot_main, INSYNC);
  robot_setopt(&robot_main, VAR_SPEED);
  robot_unsetopt(&robot_main, ADVNCD);
  robot_unsetopt(&robot_main, DISCLINANG);
  robot_unsetopt(&robot_main, NONLIN);

  // These are the prefixes to scan.
  char const *prefixes[1] = {
      "/dev/ttyUSB"}; //"/dev/ttyACM"}; // USB is the xbee
  // To enter the serial thread
  struct port_context p_cont = {prefixes, 1, 9600, 10};

  // Create the two thread structs
  pthread_t wii_thread_t;
  pthread_t serial_thread_t;

  // Creates a thread (thread_obj, parameters, description, callback function)
  thread_creator(&wii_thread_t, NULL, "Wii Controller", wii_thread);
  thread_creator(&serial_thread_t, &p_cont, "Serial Communication",
                 serial_thread);

  // Joins a thread
  thread_joiner(&wii_thread_t, "Wii Controller Thread");
  thread_joiner(&serial_thread_t, "Serial Communication thread");

  robot_clean_up(&robot_main);
  return 0;
}
