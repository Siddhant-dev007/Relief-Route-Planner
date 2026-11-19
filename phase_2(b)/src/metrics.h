#ifndef METRICS_H
#define METRICS_H

#include <stddef.h>

/*
 * Operation counters and timing used for empirical analysis.
 * Every algorithm accepts an OpStats pointer (may be NULL).
 */

typedef struct
{
    long long comparisons;     // key comparisons (sorting, greedy order)
    long long cellsFilled;     // DP table cells computed
    long long nodesExplored;   // B&B nodes taken from the frontier
    long long nodesPruned;     // B&B nodes discarded by the bound
    long long relaxations;     // Dijkstra edge relaxations
    size_t    peakBytes;       // largest auxiliary allocation
} OpStats;

void statsReset(OpStats *stats);

// Adds counters of part into total; keeps the larger peakBytes.
void statsMerge(OpStats *total, const OpStats *part);

// Sum of all counters, used as a single "operations" figure.
long long statsOperations(const OpStats *stats);

// Records an auxiliary allocation if it is the largest so far.
void statsMemory(OpStats *stats, size_t bytes);

// Monotonic-enough wall clock in milliseconds.
double timeNowMs(void);

// Formats a byte count as "512 B", "3.2 KB" or "1.5 MB".
void formatBytes(size_t bytes, char out[], size_t size);

#endif
