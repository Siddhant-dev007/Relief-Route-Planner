#include <stdio.h>
#include <time.h>

#include "metrics.h"

void statsReset(OpStats *stats)
{
    if (stats == NULL)
        return;

    stats->comparisons = 0;
    stats->cellsFilled = 0;
    stats->nodesExplored = 0;
    stats->nodesPruned = 0;
    stats->relaxations = 0;
    stats->peakBytes = 0;
}

void statsMerge(OpStats *total, const OpStats *part)
{
    if (total == NULL || part == NULL)
        return;

    total->comparisons += part->comparisons;
    total->cellsFilled += part->cellsFilled;
    total->nodesExplored += part->nodesExplored;
    total->nodesPruned += part->nodesPruned;
    total->relaxations += part->relaxations;

    if (part->peakBytes > total->peakBytes)
        total->peakBytes = part->peakBytes;
}

long long statsOperations(const OpStats *stats)
{
    if (stats == NULL)
        return 0;

    return stats->comparisons + stats->cellsFilled +
           stats->nodesExplored + stats->relaxations;
}

void statsMemory(OpStats *stats, size_t bytes)
{
    if (stats != NULL && bytes > stats->peakBytes)
        stats->peakBytes = bytes;
}

double timeNowMs(void)
{
#if defined(TIME_UTC)
    struct timespec ts;

    if (timespec_get(&ts, TIME_UTC) == TIME_UTC)
        return (double)ts.tv_sec * 1000.0 +
               (double)ts.tv_nsec / 1000000.0;
#endif

    return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

void formatBytes(size_t bytes, char out[], size_t size)
{
    if (bytes < 1024)
        snprintf(out, size, "%zu B", bytes);
    else if (bytes < 1024 * 1024)
        snprintf(out, size, "%.1f KB", bytes / 1024.0);
    else
        snprintf(out, size, "%.1f MB", bytes / (1024.0 * 1024.0));
}
