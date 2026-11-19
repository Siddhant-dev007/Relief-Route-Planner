#ifndef KNAPSACK_GREEDY_H
#define KNAPSACK_GREEDY_H

#include "common.h"
#include "metrics.h"

/*
 * Greedy 0/1 loading (baseline). Items are ordered by
 * value/weight ratio with merge sort, then taken whole while
 * they fit. Fast but not always optimal.
 *
 * Time O(n log n), auxiliary space O(n).
 * Returns the value loaded or KNAP_ERR_INPUT / KNAP_ERR_LIMIT.
 */
int knapsackGreedy(
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
);

// Ratio order used by greedy and branch and bound:
// higher value/weight first, then higher value, then lower index.
// context must point to the KnapItem array.
int compareItemsByRatio(int a, int b, const void *context);

#endif
