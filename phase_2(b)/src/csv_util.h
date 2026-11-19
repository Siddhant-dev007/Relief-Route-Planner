#ifndef CSV_UTIL_H
#define CSV_UTIL_H

#include <stdio.h>

#define CSV_LINE_SIZE   256
#define CSV_MAX_FIELDS   16

// Creates the folder if it does not exist. Returns 1 on success.
int ensureDirectory(const char path[]);

// Opens a file for writing and writes the header line.
// Returns NULL on failure.
FILE *csvOpen(const char path[], const char header[]);

// printf-style row writer; a newline is added automatically.
void csvWriteRow(FILE *file, const char format[], ...);

void csvClose(FILE *file);

// Reads one line into buffer without the newline.
// Returns 1 on success, 0 at end of file, -1 if the line is too long.
int csvReadLine(FILE *file, char buffer[], int size);

// Splits a line in place at commas and trims spaces.
// Returns the number of fields (at most maxFields).
int csvSplit(char line[], char *fields[], int maxFields);

// Parses a whole field as an integer. Returns 1 on success.
int csvParseInt(const char text[], int *value);

#endif
