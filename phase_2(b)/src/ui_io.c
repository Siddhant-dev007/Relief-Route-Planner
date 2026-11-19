#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui_io.h"

#define LINE_SIZE 128

// ---------------------------------------------
// Reads one line. Returns 0 when the line was too
// long (the rest is discarded). Exits on end of input.
// ---------------------------------------------

static int readLine(char buffer[], size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL)
    {
        printf("\nInput ended. Exiting.\n");
        exit(EXIT_SUCCESS);
    }

    char *newline = strchr(buffer, '\n');

    if (newline != NULL)
    {
        *newline = '\0';
        return 1;
    }

    if (feof(stdin))
        return 1;

    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }

    return 0;
}

static void trim(char text[])
{
    size_t length = strlen(text);

    while (length > 0 &&
           isspace((unsigned char)text[length - 1]))
        text[--length] = '\0';

    size_t start = 0;

    while (isspace((unsigned char)text[start]))
        start++;

    if (start > 0)
        memmove(text, text + start, length - start + 1);
}

int readInt(const char prompt[], int minimum, int maximum)
{
    char buffer[LINE_SIZE];

    while (1)
    {
        printf("%s", prompt);

        if (!readLine(buffer, sizeof(buffer)))
        {
            printf("ERROR: Input too long.\n");
            continue;
        }

        trim(buffer);

        char *end;
        long value = strtol(buffer, &end, 10);

        if (end == buffer || *end != '\0')
        {
            printf("ERROR: Enter a whole number.\n");
            continue;
        }

        if (value < minimum || value > maximum)
        {
            printf("ERROR: Enter a number from %d to %d.\n",
                   minimum, maximum);
            continue;
        }

        return (int)value;
    }
}

void readText(const char prompt[], char out[], size_t size)
{
    char buffer[LINE_SIZE];

    while (1)
    {
        printf("%s", prompt);

        if (!readLine(buffer, sizeof(buffer)))
        {
            printf("ERROR: Input too long.\n");
            continue;
        }

        trim(buffer);

        if (buffer[0] == '\0')
        {
            printf("ERROR: Value cannot be empty.\n");
            continue;
        }

        if (strchr(buffer, ',') != NULL)
        {
            printf("ERROR: Commas are not allowed.\n");
            continue;
        }

        if (strlen(buffer) >= size)
        {
            printf("ERROR: Use at most %zu characters.\n",
                   size - 1);
            continue;
        }

        strcpy(out, buffer);
        return;
    }
}

int readYesNo(const char prompt[])
{
    char buffer[LINE_SIZE];

    while (1)
    {
        printf("%s", prompt);

        if (!readLine(buffer, sizeof(buffer)))
            continue;

        trim(buffer);

        char answer = (char)tolower((unsigned char)buffer[0]);

        if (buffer[0] != '\0' && buffer[1] == '\0')
        {
            if (answer == 'y')
                return 1;
            if (answer == 'n')
                return 0;
        }

        printf("ERROR: Enter y or n.\n");
    }
}

void printRule(int width)
{
    for (int i = 0; i < width; i++)
        putchar('-');

    putchar('\n');
}

void heading(const char title[])
{
    printf("\n");
    printf("==================================================\n");
    printf(" %s\n", title);
    printf("==================================================\n");
}

void pause(void)
{
    char buffer[LINE_SIZE];

    printf("\nPress Enter to continue...");
    readLine(buffer, sizeof(buffer));
}
