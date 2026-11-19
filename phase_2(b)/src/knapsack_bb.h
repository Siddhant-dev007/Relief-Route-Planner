#ifndef KNAPSACK_BB_H
#define KNAPSACK_BB_H

#include "common.h"
#include "metrics.h"

/*
 * 0/1 knapsack by least-cost branch and bound (exact alternative).
 *
 * Each node fixes include/exclude for the next item in ratio order.
 * Its bound is the fractional-knapsack value of the remaining items,
 * an upper limit on any completion. The frontier is a max-heap on
 * the bound (least cost = -bound), and nodes whose bound cannot
 * beat the best value found so far are pruned.
 *
 * Always optimal. Worst case O(2^n) nodes; usually far fewer.
 * Returns the best value, KNAP_ERR_INPUT or KNAP_ERR_LIMIT
 * (more than MAX_BB_NODES nodes would be needed).
 */
int knapsackBranchBound(
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
);

#endif
