#ifndef KNAPSACK_DP_H
#define KNAPSACK_DP_H

#include "common.h"
#include "metrics.h"

/*
 * 0/1 knapsack by dynamic programming (primary algorithm, Phase I).
 *
 * dp[i][w] = best value using the first i items with capacity w
 * dp[i][w] = max(dp[i-1][w], value_i + dp[i-1][w - weight_i])
 *
 * Always optimal. Time O(nW), auxiliary space O(nW).
 * selected[i] is set to 1 for chosen items.
 * Returns the best value, KNAP_ERR_INPUT or KNAP_ERR_LIMIT.
 */
int knapsackDP(
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
);

// Shared input check for all three loading algorithms.
int knapsackInputValid(const KnapItem items[], int n, int capacity);

#endif
