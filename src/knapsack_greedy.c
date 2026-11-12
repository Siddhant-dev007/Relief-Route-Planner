#include <stdlib.h>

#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "sort_search.h"

int compareItemsByRatio(int a, int b, const void *context)
{
    const KnapItem *items = context;

    // Compare value_a / weight_a with value_b / weight_b
    // by cross-multiplying, so no floating point is needed.
    long long left = (long long)items[a].value * items[b].weight;
    long long right = (long long)items[b].value * items[a].weight;

    if (left != right)
        return left > right ? -1 : 1;

    if (items[a].value != items[b].value)
        return items[a].value > items[b].value ? -1 : 1;

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
    if (!knapsackInputValid(items, n, capacity) || selected == NULL)
        return KNAP_ERR_INPUT;

    for (int i = 0; i < n; i++)
        selected[i] = 0;

    if (n == 0 || capacity == 0)
        return 0;

    int *order = malloc((size_t)n * sizeof(int));

    if (order == NULL)
        return KNAP_ERR_LIMIT;

    for (int i = 0; i < n; i++)
        order[i] = i;

    if (!mergeSortIndices(order, n, compareItemsByRatio,
                          items, stats))
    {
        free(order);
        return KNAP_ERR_LIMIT;
    }

    // order[] plus the merge-sort scratch array.
    statsMemory(stats, 2 * (size_t)n * sizeof(int));

    int remaining = capacity;
    int total = 0;

    for (int k = 0; k < n; k++)
    {
        int i = order[k];

        if (stats != NULL)
            stats->comparisons++;

        if (items[i].weight <= remaining)
        {
            selected[i] = 1;
            remaining -= items[i].weight;
            total += items[i].value;
        }
    }

    free(order);
    return total;
}
