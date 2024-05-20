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

int message_accuracy; // Incriment this everytime we get a message
int partial_messages;
int failed_messages;

int main() {

  message_accuracy = 0;
  partial_messages = 0;
  failed_messages = 0;

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

  char buffer[20];

  for (int i = 0; i < NUM_MESSAGES; ++i) {
    int a = read(fd, buffer, MESSAGE_SIZE_BYTES);
    if (a == -1)
      failed_messages += 1;
    else if (a < MESSAGE_SIZE_BYTES)
      partial_messages += 1;
    else
      message_accuracy += 1;
    if (buffer[0] != FIRST)
      printf("Dropped %s\n", FIRST);
    if (buffer[1] != SECOND)
      printf("Dropped %s\n", SECOND);
    if (buffer[2] != THIRD)
      printf("Dropped %s\n", THIRD);
    usleep(1000); // Sleep 1 millisecond
  }

  printf("Read from %d messages, of which  \n"
         "%d >> where correct,\n"
         "%d >> were partially correct \n"
         "%d >> failed to read\n",
         NUM_MESSAGES, message_accuracy, partial_messages, failed_messages);
}
