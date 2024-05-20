/**
 * @author      : theo (theo@$HOSTNAME)
 * @file        : xbee_host
 * @created     : Monday Aug 19, 2019 09:06:17 MDT
 */

#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "globals.h"

#define MESSAGE_SIZE_BYTES                                                     \
  3 // 3 char message (or int8_t), make sure we get each one
#define NUM_MESSAGES 10000
#define FIRST 'd'
#define SECOND 't'
#define THIRD 'h'

int main() {

  if (access(XBEE_PORT, F_OK) == -1) {
    printf("USB Not file\n");
    return -1;
  }

  int fd;
  struct termios toptions;
  fd = open(XBEE_PORT, O_RDWR | O_NOCTTY);
  if (fd == -1) {
    perror("init serial port unable to open port\n");
    return -1;
  }

  if (tcgetattr(fd, &toptions) < 0) {
    perror("error couldnt set term attributes\n");
    return -1;
  }

  speed_t brate = BAUD;

  cfsetispeed(&toptions, brate);
  cfsetospeed(&toptions, brate);

  toptions.c_cflag = toptions.c_cflag & ~(PARENB | CSTOPB | CSIZE | CRTSCTS) |
                     CREAD | CLOCAL | CS8;

  toptions.c_iflag = toptions.c_iflag & ~(IXON | IXOFF | IXANY);

  toptions.c_lflag = toptions.c_lflag & ~(ICANON | ECHO | ECHOE | ISIG);

  toptions.c_oflag = toptions.c_oflag & ~OPOST;

  toptions.c_cc[VMIN] = 0;   // Min bytes
  toptions.c_cc[VTIME] = 20; // 2 seconds before read returns (20 * .1)

  if (tcsetattr(fd, TCSANOW, &toptions) < 0) {
    perror("couldn't set term attributes");
    return -1;
  }

  char buffer[3];
  buffer[0] = FIRST;
  buffer[1] = SECOND;
  buffer[2] = THIRD;
  char buffer2[256];

  /** for(int i = 0; i < NUM_MESSAGES; ++i) { */
  int a = write(fd, "m", 1);
  a = write(fd, "h", 1);
  a = write(fd, "h", 1);
  buffer2[6] = '\0';
  /** } */

  if (a == -1)
    printf("NOOOO\n");
  else
    printf("YESSS\n");

  close(fd);
}
