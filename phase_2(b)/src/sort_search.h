#ifndef SORT_SEARCH_H
#define SORT_SEARCH_H

#include "metrics.h"

/*
 * Merge sort over an array of indices, ordered by a caller-supplied
 * comparator, and iterative binary search over sorted integers.
 *
 * Sorting indices (not records) lets the same routine order packages,
 * shelters and vehicles without copying the records themselves.
 */

// Returns < 0 if a should come before b, > 0 if after, 0 if equal.
typedef int (*IndexCompare)(int a, int b, const void *context);

// Stable merge sort. Returns 1 on success, 0 if memory ran out.
// Time O(n log n), auxiliary space O(n).
int mergeSortIndices(
    int indices[],
    int n,
    IndexCompare compare,
    const void *context,
    OpStats *stats
);

// Returns the position of key in sorted[0..n-1], or -1.
// Time O(log n), auxiliary space O(1).
int binarySearchInt(
    const int sorted[],
    int n,
    int key,
    OpStats *stats
);

#endif
