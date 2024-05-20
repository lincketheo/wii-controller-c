/**
 * @file Utils.h
 * @brief A File with thread synchronization methods and signal handlers
 *
 * This file controls the main control loops and threads for robot control flow
 *
 * @author      Theo (theo@theo-Lenovo-Yoga-Arch)
 * @created     Monday Aug 05, 2019 12:04:40 MDT
 * @bugs        No known bugs
 */

#ifndef UTILS_H
#define UTILS_H

// C Includes
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse.h>

// Local Includes
#include "kermit.h"
#include "log.h"
#include "robot_control.h"
#include "serial.h"
#include "wii_controller.h"

#define DRIVE 'd'
#define STOP 's'
#define TRIGGER 't'

#define SERIAL_PERIOD 0.1
#define PERIOD_CONV 1000000

/////////////////////////////////////////// Globals
///////////////////////////////////////////////////

/**
 * Signals
 *
 * @scan_signal A signal triggered ON to stop a scanner
 * @running     A signal triggered off to stop a running serial / wii remote
 * thread
 */
volatile int scan_signal;
volatile int running;

/**
 * Thread synchronizers
 *
 * The serial thread waits until wii is ready
 * and the wii thread waits for the serial thread
 */
volatile int wii_ready;
volatile int serial_ready;

/**
 * The main global robot data structure element
 *
 * This is the only shared data structure that could feature race conditions
 * I declare it here to denote that (I don't need it global, but it helps to
 * see which data is subjected to race conditions)
 */
struct robot_s robot_main;

//////////////////////////////////////////// Data Strucutres
///////////////////////////////////////////
// Context for writing to the serial
struct serial_context {
  int fd;
  struct robot_s robot;
};

struct robot_context {
  struct robot_s *robot;
  struct controller_s *controller;
  wiimote **wiimotes;
  int verbose;
};

// Context for serial scanner thread
struct port_context {
  const char **const prefixes;
  const int size;
  const int baud;
  const int num_ports_to_scan;
};

////////////////////////////////////////// Forward Declaration
///////////////////////////////////////////

/**
 * @brief Handle Signals
 *
 * @note For now, it just sets both scan_signal and running
 * In the future though, this is were we would define conditions for the
 * type of signal
 *
 * this is setup with the call
 * signal(SIGINT, signal_handler); // or another signal
 * @param signal The signal recieved
 */
void signal_hander(int signal);

/**
 * @brief Scans a single prefix through specified ranges
 *
 * @param prefix The prefix to the port (ex "/dev/ttyUSB" or "/dev/ttyACM")
 * @param baud Serial baud - default 9600
 * @param num_ports_to_scan Number of ports to scan (ex "/dev/ttyUSB0 -
 * /dev/ttyUSB29" is 30)
 *
 * @return The file descriptor this cannot be used alone because you must close
 * fd
 */
int scan_serial(const char *const prefix, const int baud,
                int num_ports_to_scan);

/**
 * @brief Scans serial ports with a list of prefixes
 * @note, this is almost a serial function because I was origionally going to
 * make it one, but alas nah
 *
 * @param context A port_context struct
 *
 * @return The fd to the new port, mst be closed
 */
int serial_scanner_thread(void *context);

/**
 * @brief The robot command to write to the serial //TODO this probably doesn't
 * belong here
 *
 * @param msg A string message, for now, I'm just treating this like a single
 * char
 * @param fd The file to write to
 * @param robot The robot to get data from
 */
void write_to_serial(char *msg, int fd, struct robot_s *robot);

/**
 * @brief The main serial thread
 * @note This thread opens AND closes the file descriptor, a choice a lot of
 * programmers may think is stupid
 *
 * @param context The serial_context
 *
 * @return Make the compiler happy
 */
void *serial_thread(void *context);

/**
 * @brief Scans for wiimotes and does it again
 * @note This should never return NULL
 *
 * @return an array of wiimotes
 */
wiimote **scan_wii();

/**
 * @brief The main wii thread
 * @note all wii stuff has a lifetime in this function
 *
 * @return Make the compiler happy
 */
void *wii_thread(void *);

/**
 * @brief A helper for thread management
 *
 * @param thread The pthread_t to put the thread into
 * @param context The void* context
 * @param thread_desc The thread description
 * @param thread_fnc The function to execute
 *
 * @return status
 */
int thread_creator(pthread_t *thread, void *context, const char *thread_desc,
                   void *(*thread_fnc)(void *));

/**
 * @brief A helper for thread joining
 *
 * @param thread The thread to join
 * @param thread_desc The description of the thread
 *
 * @return status
 */
int thread_joiner(pthread_t *thread, const char *thread_desc);

//////////////////////////////////////////////////// Func Definicitons
///////////////////////////////////

void signal_handler(int sig) {
  pthread_mutex_lock(&p_lock);
  log_info("Recieved signal: %d", sig);
  scan_signal = 1;
  running = 0;
  pthread_mutex_unlock(&p_lock);
}

int scan_serial(const char *const prefix, const int baud,
                int num_ports_to_scan) {
  int fd;
  int size;

  // Checks the size in digits of num_ports_to_scan - kind of awkward, might
  // take out
  {
    char snum[100];
    size = sprintf(snum, "%d", num_ports_to_scan) + strlen(prefix);
  }

  char port[size];
  int i;

  for (i = 0; i < num_ports_to_scan && !scan_signal; ++i) {
    sprintf(port, "%s%d", prefix, i);
    fd = new_serial_port(port, baud);
    if (fd != -1)
      return fd; // A try catch might be better here
  }
  return fd;
}

int serial_scanner_thread(void *context) {
  struct port_context *prefixes_to_scan = (struct port_context *)context;
  int fd;

  int size = prefixes_to_scan->size;
  int baud = prefixes_to_scan->baud;
  int num_ports = prefixes_to_scan->num_ports_to_scan;

  do {
    for (int i = 0; i < size && fd == -1; ++i) {
      fd = scan_serial(prefixes_to_scan->prefixes[i], baud, num_ports);
      if (scan_signal) {
        return -1;
      }
      log_info("Scanned %d ports", num_ports);
      sleep(1);
    }
  } while (fd == -1);
  if (scan_signal) {
    return -1;
  }
  serial_ready = 1;
  return fd;
}

// This will change
void write_to_serial(char *msg, int fd, struct robot_s *robot) {
  int ret;
  msg[0] = DRIVE;
  msg[1] = 2 * robot->drive->linear_vel;
  msg[2] = 2 * robot->drive->angular_vel;
  msg[3] = 2 * robot->gun->left_mag;
  msg[4] = 2 * robot->gun->right_mag;

  ret = write(fd, msg, 5);

  if (ret == -1) {
    // https://linux.die.net/man/2/write
    switch (errno) {
    case EBADF: {
      log_error("Error %d closing fd and exiting safely: %s", errno,
                strerror(errno));
      running = 0;
      break;
    }
    case EIO: {
      log_error("Error %d: %s", errno, strerror(errno));
      log_info("Trying one more time for input output error");
      if (write(fd, msg, 5) == -1) {
        log_error("Failed again with error %d, exiting thread", errno);
        running = 0;
      }
      break;
    }
    default: {
      log_error("Error %d: %s", errno, strerror(errno));
      log_info("Trying one more time");
      if (write(fd, msg, 3) == -1) {
        log_info("Failed again with error %d, exiting thread", errno);
        running = 0;
      }
      break;
    }
    }
  }
}

void *serial_thread(void *context) {
  // Grab that context
  struct port_context *port_cont = (struct port_context *)context;
  int fd = -1;

  if (!(robot_main.options & DEBUG)) {
    // Underlying file descriptor
    fd = serial_scanner_thread(port_cont);
    if (fd != -1)
      log_info("Successfully found fd");
    else
      log_error("Invalid file desc");
  } else {
    serial_ready = 1;
  }
  struct serial_context cont = {fd, robot_main};

  char msg[5];
  for (; running && !wii_ready;)
    ;
  while (running) {
    if (!(cont.robot.options & DEBUG)) {
      write_to_serial(msg, cont.fd, &robot_main); // handles errors here
    }
    usleep(SERIAL_PERIOD * PERIOD_CONV);
  }
  if (!(robot_main.options & DEBUG))
    close(fd);
  log_info("Safely closed file descriptor");
  return NULL;
}

wiimote **scan_wii() {
  wiimote **wiimotes;
  do {
    wiimotes = wiimote_init();
    if (scan_signal) {
      return NULL;
    }
    sleep(1);
  } while (!wiimotes);
  if (scan_signal) {
    return NULL;
  }
  wii_ready = 1;
  return wiimotes;
}

void *wii_thread(void *context) {

  // Create new wiimotes by scanning - this should never by nullptr
  wiimote **wiimotes = scan_wii();

  struct controller_s controller = kermit_controller();

  struct robot_context robot_cont = {&robot_main, &controller, wiimotes};

  // Wait for serial to be ready
  for (; running && !serial_ready;)
    ;
  while (running && heart_beat(robot_cont.wiimotes, 1)) {
    event_loop(robot_cont.wiimotes, robot_cont.robot, robot_cont.controller);
    usleep(robot_cont.robot->period * PERIOD_CONV);
  }
  if (wiimotes)
    wiiuse_cleanup(robot_cont.wiimotes, 1); // change number later, I'm lazy
  return NULL;
}

int thread_creator(pthread_t *thread, void *context, const char *thread_desc,
                   void *(*thread_fnc)(void *)) {
  if (pthread_create(thread, NULL, thread_fnc, context)) {
    log_error("Error creating %s thread", thread_desc);
    return -1;
  } else {
    log_info("Created %s thread successfully", thread_desc);
  }
  return 0;
}

int thread_joiner(pthread_t *thread, const char *thread_desc) {
  if (pthread_join(*thread, NULL)) {
    log_info("Error joining %s thread", thread_desc);
    return -1;
  } else {
    log_info("Joined %s thread successfully", thread_desc);
  }
  return 0;
}

#endif
