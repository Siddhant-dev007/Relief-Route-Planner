#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MAKE_DIR(path) mkdir(path, 0755)
#endif

#include "csv_util.h"

int ensureDirectory(const char path[])
{
    if (MAKE_DIR(path) == 0 || errno == EEXIST)
        return 1;

    return 0;
}

FILE *csvOpen(const char path[], const char header[])
{
    FILE *file = fopen(path, "w");

    if (file == NULL)
        return NULL;

    fprintf(file, "%s\n", header);
    return file;
}

void csvWriteRow(FILE *file, const char format[], ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fputc('\n', file);
}

void csvClose(FILE *file)
{
    if (file != NULL)
        fclose(file);
}

int csvReadLine(FILE *file, char buffer[], int size)
{
    if (fgets(buffer, size, file) == NULL)
        return 0;

    char *newline = strchr(buffer, '\n');

    if (newline == NULL && !feof(file))
    {
        int ch;

        while ((ch = fgetc(file)) != '\n' && ch != EOF)
        {
        }

        return -1;
    }

    if (newline != NULL)
        *newline = '\0';

    // Remove a Windows carriage return.
    size_t length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\r')
        buffer[length - 1] = '\0';

    return 1;
}

static char *trimField(char *text)
{
    while (isspace((unsigned char)*text))
        text++;

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char)end[-1]))
        *--end = '\0';

    return text;
}

int csvSplit(char line[], char *fields[], int maxFields)
{
    int count = 0;
    char *start = line;

    while (count < maxFields)
    {
        char *comma = strchr(start, ',');

        if (comma != NULL)
            *comma = '\0';

        fields[count++] = trimField(start);

        if (comma == NULL)
            break;

        start = comma + 1;
    }

    return count;
}

int csvParseInt(const char text[], int *value)
{
    char *end;

    errno = 0;
    long parsed = strtol(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE ||
        parsed < INT_MIN || parsed > INT_MAX)
        return 0;

    *value = (int)parsed;
    return 1;
}
