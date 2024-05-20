/**
 * @author      : theo (theo@varnsen)
 * @file        : string_help
 * @brief
 *
 * This is used for logging to shorten file names
 *
 * @created     : Friday Aug 09, 2019 16:31:06 MDT
 * @bugs
 */

#ifndef STRING_HELP_H

#define STRING_HELP_H

// C Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local Includes

/**
 * @brief Cuts a filename to num_backwards files
 *
 * example:
 * char buffer[20];
 * cut_file_name(buffer, /home/theo/files/other/files/more/files, 3)
 * buffer[18] = '\0';
 * printf("%s\n", buffer);
 *
 * >> /files/more/files
 *
 *
 * @param buffer A buffer to store the final string
 * @param filename The name of the file (root or not)
 * @param num_backwards Number of directories to include
 */
void cut_file_name(char *buffer, const char *filename, int num_backwards);

#endif /* end of include guard STRING_HELP_H */
