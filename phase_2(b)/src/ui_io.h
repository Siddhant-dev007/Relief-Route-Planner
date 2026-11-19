#ifndef UI_IO_H
#define UI_IO_H

#include <stddef.h>

/*
 * Console input/output helpers. All user input goes through
 * these functions so validation is done in one place.
 * End of input (Ctrl+Z / closed pipe) exits the program cleanly.
 */

// Reads an integer in [minimum, maximum]; re-prompts on bad input.
int readInt(const char prompt[], int minimum, int maximum);

// Reads a non-empty line without commas (keeps CSV exports safe).
void readText(const char prompt[], char out[], size_t size);

// Reads y/n; returns 1 for yes, 0 for no.
int readYesNo(const char prompt[]);

// Prints a boxed section title.
void heading(const char title[]);

// Prints a horizontal rule of the given width.
void printRule(int width);

// Waits for Enter.
void pause(void);

#endif
