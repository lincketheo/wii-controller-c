/**
 * @author                : theo (theo@varsen)
 * @file                  : serial
 * @brief A small set of utility functions for serial communication
 *
 * This is for serial specific communication
 *
 * @created               : Friday Aug 09, 2019 14:01:50 MDT
 * @bugs No Known Bugs
 */
#ifndef SERIAL_H
#define SERIAL_H

// C Includes
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

// Local Includes

//////////////////////////////////// Forward Decleration
////////////////////////////////////////
/**
 * @brief Creates a new serial port opening with some default settings
 *
 * @param port The port to open (ex /dev/ttyUSB)
 * @param baud The baud to communicate with (an int converts to a new baud type
 *
 * @return The file descriptor
 */
int new_serial_port(const char *port, int baud);

//////////////////////////////////// Definitions
////////////////////////////////////////////////
int new_serial_port(const char *port, int baud) {
  if (access(port, F_OK) == -1)
    return -1;
  int fd;
  struct termios toptions;

  fd = open(port, O_RDWR | O_NOCTTY);

  if (fd == -1) {
    perror("init_serialport error : unable to open port \n");
    return -1;
  }

  if (tcgetattr(fd, &toptions) < 0) {
    perror("init_serialport error : Couldn't get term attribtues\n");
    return -1;
  }

  speed_t brate = baud; // let you override switch below if needed
  switch (baud) {
  case 4800:
    brate = B4800;
    break;
  case 9600:
    brate = B9600;
    break;
  case 19200:
    brate = B19200;
    break;
  case 38400:
    brate = B38400;
    break;
  case 57600:
    brate = B57600;
    break;
  case 115200:
    brate = B115200;
    break;
  }

  cfsetispeed(&toptions, brate);
  cfsetospeed(&toptions, brate);

  // Options
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  // no flow control
  toptions.c_cflag &= ~CRTSCTS;
  toptions.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines
  toptions.c_iflag &= ~(IXON | IXOFF | IXANY);         // turn off s/w flow ctrl
  toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  toptions.c_oflag &= ~OPOST;                          // make raw

  // http://unixwiz.net/techtips/termios-vmin-vtime.html
  toptions.c_cc[VMIN] = 0;
  toptions.c_cc[VTIME] = 20;
  if (tcsetattr(fd, TCSANOW, &toptions) < 0) {
    perror("init_serialport: Couldn't set term attributes");
    return -1;
  }
  return fd;
}

#endif
