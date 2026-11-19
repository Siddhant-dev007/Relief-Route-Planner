#include "greedy.h"


int greedyAllocation(
    SupplyPackage packages[],
    int n,
    int capacity,
    int selected[]
)
{
    // Stores package indices so that the original
    // package array does not have to be rearranged
    int order[MAX_PACKAGES];

    float ratio[MAX_PACKAGES];


    // -----------------------------------------
    // Calculate Relief Value / Weight ratio
    // -----------------------------------------

    for (int i = 0; i < n; i++)
    {
        ratio[i] =
            (float) packages[i].reliefValue /
            packages[i].weight;

        order[i] = i;

        selected[i] = 0;
    }


    // -----------------------------------------
    // Sort package indices in decreasing order
    // of Relief Value / Weight ratio
    //
    // Simple selection sort is used here so
    // that the sorting logic is implemented
    // manually.
    // -----------------------------------------

    for (int i = 0; i < n - 1; i++)
    {
        int maxPosition = i;

        for (int j = i + 1; j < n; j++)
        {
            if (ratio[order[j]] >
                ratio[order[maxPosition]])
            {
                maxPosition = j;
            }
        }


        // Swap indices
        int temp = order[i];

        order[i] = order[maxPosition];

        order[maxPosition] = temp;
    }


    // -----------------------------------------
    // Select complete packages greedily
    // -----------------------------------------

    int remainingCapacity = capacity;

    int totalReliefValue = 0;


    for (int i = 0; i < n; i++)
    {
        int packageIndex = order[i];


        if (packages[packageIndex].weight
            <= remainingCapacity)
        {
            selected[packageIndex] = 1;


            remainingCapacity -=
                packages[packageIndex].weight;


            totalReliefValue +=
                packages[packageIndex].reliefValue;
        }
    }


    return totalReliefValue;
}