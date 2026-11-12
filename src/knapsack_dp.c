#include <stdlib.h>

#include "knapsack_dp.h"

int knapsackInputValid(const KnapItem items[], int n, int capacity)
{
    if (n < 0 || n > MAX_ITEMS || capacity < 0)
        return 0;

    if (n > 0 && items == NULL)
        return 0;

    for (int i = 0; i < n; i++)
    {
        if (items[i].weight <= 0 || items[i].value < 0)
            return 0;
    }

    return 1;
}

int knapsackDP(
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

    long long cells = (long long)(n + 1) * (capacity + 1);

    if (cells > MAX_DP_CELLS)
        return KNAP_ERR_LIMIT;

    // One flat block: row i starts at i * (capacity + 1).
    int *table = malloc((size_t)cells * sizeof(int));

    if (table == NULL)
        return KNAP_ERR_LIMIT;

    statsMemory(stats, (size_t)cells * sizeof(int));

    int width = capacity + 1;

    // Base case: no items gives value 0 for every capacity.
    for (int w = 0; w <= capacity; w++)
        table[w] = 0;

    for (int i = 1; i <= n; i++)
    {
        int *row = &table[(size_t)i * width];
        int *previous = &table[(size_t)(i - 1) * width];
        int weight = items[i - 1].weight;
        int value = items[i - 1].value;

        row[0] = 0;

        for (int w = 1; w <= capacity; w++)
        {
            int exclude = previous[w];

            if (weight <= w)
            {
                int include = value + previous[w - weight];
                row[w] = include > exclude ? include : exclude;
            }
            else
            {
                row[w] = exclude;
            }
        }
    }

    if (stats != NULL)
        stats->cellsFilled += (long long)n * capacity;

    int best = table[(size_t)n * width + capacity];

    // Backtrack: an item was taken when its row
    // improved on the row above at the current capacity.
    int w = capacity;

    for (int i = n; i > 0; i--)
    {
        if (table[(size_t)i * width + w] !=
            table[(size_t)(i - 1) * width + w])
        {
            selected[i - 1] = 1;
            w -= items[i - 1].weight;
        }
    }

    free(table);
    return best;
}
