/*
 * Greedy 0/1 knapsack.
 *
 * Idea: put the "best value per kg" packages on the truck first.
 *   Step 1  list the packages: 0, 1, 2, ... n-1
 *   Step 2  sort that list, best value-per-kg first (merge sort)
 *   Step 3  walk the list; load a package if it still fits
 *
 * Fast, but not always the best load (see the greedy trap).
 */

#include "knapsack_dp.h"       // knapsackInputValid
#include "knapsack_greedy.h"
#include "sort_search.h"       // mergeSortIndices

/*
 * Which of packages a and b should come first?
 * Returns negative if a goes first, positive if b goes first.
 *
 * We want to compare   value_a / weight_a   with   value_b / weight_b.
 * Dividing gives decimals, which can round badly, so we multiply
 * both sides by (weight_a * weight_b) instead:
 *
 *      value_a * weight_b   vs   value_b * weight_a
 *
 * Example: a = 90/50 (1.8), b = 100/51 (1.96)
 *          90*51 = 4590  <  100*50 = 5000   so b goes first.
 *
 * Used by greedy and by branch and bound.
 */
int compareItemsByRatio(int a, int b, const void *context)
{
    const KnapItem *items = context;   // the package list being sorted

    long long scoreA = (long long)items[a].value * items[b].weight;
    long long scoreB = (long long)items[b].value * items[a].weight;

    if (scoreA > scoreB) return -1;    // a has the better ratio
    if (scoreA < scoreB) return 1;     // b has the better ratio

    // Same ratio: higher value first.
    if (items[a].value > items[b].value) return -1;
    if (items[a].value < items[b].value) return 1;

    // Still equal: keep the original order.
    return a - b;
}

int knapsackGreedy(
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
)
{
    // Reject bad input (weight <= 0, n > 1000, ...).
    if (!knapsackInputValid(items, n, capacity) || selected == NULL)
        return KNAP_ERR_INPUT;

    // Nothing loaded yet.
    for (int i = 0; i < n; i++)
        selected[i] = 0;

    // No packages or an empty truck: nothing to do.
    if (n == 0 || capacity == 0)
        return 0;

    // Step 1: list the package numbers 0..n-1.
    int order[MAX_ITEMS];

    for (int i = 0; i < n; i++)
        order[i] = i;

    // Step 2: sort the list, best value-per-kg first.
    if (!mergeSortIndices(order, n, compareItemsByRatio, items, stats))
        return KNAP_ERR_LIMIT;   // merge sort could not get memory

    statsMemory(stats, 2 * (size_t)n * sizeof(int));  // list + sort buffer

    // Step 3: load each package if it still fits.
    int spaceLeft = capacity;
    int totalValue = 0;

    for (int k = 0; k < n; k++)
    {
        int p = order[k];   // k-th best package

        if (stats != NULL)
            stats->comparisons++;

        if (items[p].weight <= spaceLeft)
        {
            selected[p] = 1;
            spaceLeft -= items[p].weight;
            totalValue += items[p].value;
        }
    }

    return totalValue;
}
