/**
 * @author      : theo (theo@$HOSTNAME)
 * @file        : string_ops
 * @created     : Wednesday Aug 21, 2019 15:40:42 MDT
 */

#include "string_ops.h"

void cut_file_name(char *buffer, const char *filename, int num_backwards) {
  int index = strlen(filename) - 1;
  int copy_index;

  int num_slashes = 0;
  int i;

  for (i = index; num_slashes < num_backwards && i >= 0; --i)
    if (filename[i] == '/')
      num_slashes++;

  copy_index = i;
  memcpy(buffer, &filename[copy_index + 1], index - copy_index);
  buffer[index - copy_index] = '\0';
}
