#include "knapsack.h"


int knapsackDP(
    SupplyPackage packages[],
    int n,
    int capacity,
    int selected[]
)
{
    int dp[MAX_PACKAGES + 1][capacity + 1];


    // Base cases
    for (int i = 0; i <= n; i++)
    {
        dp[i][0] = 0;
    }

    for (int w = 0; w <= capacity; w++)
    {
        dp[0][w] = 0;
    }


    // Fill DP table
    for (int i = 1; i <= n; i++)
    {
        for (int w = 1; w <= capacity; w++)
        {
            if (packages[i - 1].weight <= w)
            {
                int include =
                    packages[i - 1].reliefValue +
                    dp[i - 1]
                      [w - packages[i - 1].weight];

                int exclude =
                    dp[i - 1][w];

                if (include > exclude)
                {
                    dp[i][w] = include;
                }
                else
                {
                    dp[i][w] = exclude;
                }
            }
            else
            {
                dp[i][w] =
                    dp[i - 1][w];
            }
        }
    }


    // Initially nothing selected
    for (int i = 0; i < n; i++)
    {
        selected[i] = 0;
    }


    // Backtrack
    int w = capacity;

    for (int i = n; i > 0; i--)
    {
        if (dp[i][w] != dp[i - 1][w])
        {
            selected[i - 1] = 1;

            w -= packages[i - 1].weight;
        }
    }


    return dp[n][capacity];
}