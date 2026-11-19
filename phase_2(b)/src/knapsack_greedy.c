#include <stdlib.h>

#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "sort_search.h"


// Compare two items according to value/weight ratio
int compareItemsByRatio(int a, int b, const void *context)
{
    const KnapItem *items = context;

    double ratioA = (double)items[a].value / items[a].weight;
    double ratioB = (double)items[b].value / items[b].weight;

    // Higher ratio should come first
    if (ratioA > ratioB)
        return -1;

    if (ratioA < ratioB)
        return 1;

    // If ratios are same, item with higher value comes first
    if (items[a].value > items[b].value)
        return -1;

    if (items[a].value < items[b].value)
        return 1;

    // If everything is same, smaller index comes first
    if (a < b)
        return -1;

    if (a > b)
        return 1;

    return 0;
}


int knapsackGreedy(
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
)
{
    // Check input
    if (!knapsackInputValid(items, n, capacity) || selected == NULL)
        return KNAP_ERR_INPUT;

    // Initially no item is selected
    for (int i = 0; i < n; i++)
        selected[i] = 0;

    if (n == 0 || capacity == 0)
        return 0;


    // order[] will store item indices
    int *order = malloc((size_t)n * sizeof(int));

    if (order == NULL)
        return KNAP_ERR_LIMIT;


    // Initially: 0, 1, 2, 3, ...
    for (int i = 0; i < n; i++)
        order[i] = i;


    // Sort item indices according to value/weight ratio
    if (!mergeSortIndices(order, n, compareItemsByRatio,
                          items, stats))
    {
        free(order);
        return KNAP_ERR_LIMIT;
    }


    statsMemory(stats, 2 * (size_t)n * sizeof(int));

    int remaining = capacity;
    int total = 0;


    // Check items from highest ratio to lowest ratio
    for (int k = 0; k < n; k++)
    {
        int i = order[k];

        if (stats != NULL)
            stats->comparisons++;

        // Take item if it fits
        if (items[i].weight <= remaining)
        {
            selected[i] = 1;

            remaining = remaining - items[i].weight;
            total = total + items[i].value;
        }
    }


    free(order);

    return total;
}
