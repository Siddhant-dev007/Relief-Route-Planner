#include <stdio.h>

#include "relief.h"
#include "knapsack.h"
#include "greedy.h"
#include "validation.h"


// =====================================================
// DISPLAY SELECTED PACKAGES
// =====================================================

void displayAllocation(
    char algorithmName[],
    SupplyPackage packages[],
    int n,
    int selected[],
    int capacity,
    int totalReliefValue
)
{
    printf("\n=========================================\n");
    printf(" %s\n", algorithmName);
    printf("=========================================\n");

    int totalWeight = 0;

    printf("\nSelected Packages:\n");

    for (int i = 0; i < n; i++)
    {
        if (selected[i] == 1)
        {
            printf(
                "%s | Weight: %d kg | Relief Value: %d\n",
                packages[i].type,
                packages[i].weight,
                packages[i].reliefValue
            );

            totalWeight += packages[i].weight;
        }
    }

    // If no package could be selected
    if (totalWeight == 0)
    {
        printf("None\n");
    }

    printf("\nTotal Weight Used: %d kg\n",
           totalWeight);

    printf("Remaining Capacity: %d kg\n",
           capacity - totalWeight);

    printf("Total Relief Value: %d\n",
           totalReliefValue);
}


// =====================================================
// MAIN FUNCTION
// =====================================================

int main()
{
    Shelter shelter;

    SupplyPackage packages[MAX_PACKAGES];

    Vehicle vehicle;

    int selectedDP[MAX_PACKAGES];
    int selectedGreedy[MAX_PACKAGES];

    int n;
    int choice;


    // =================================================
    // PROGRAM HEADING
    // =================================================

    printf("\n=========================================\n");
    printf(" DISASTER RELIEF SUPPLY DISTRIBUTION\n");
    printf("=========================================\n");


    // =================================================
    // SHELTER INPUT
    // =================================================

    inputShelter(&shelter);


    // =================================================
    // PACKAGE INPUT
    // =================================================

    printf("\nNumber of Supply Packages: ");
    scanf("%d", &n);


    // Validate number of packages
    if (!validatePackageCount(n))
    {
        printf(
            "\nERROR: Number of packages must be "
            "between 1 and %d.\n",
            MAX_PACKAGES
        );

        return 1;
    }


    inputPackages(
        packages,
        n,
        shelter.urgency
    );


    displayPackages(
        packages,
        n
    );


    // =================================================
    // VEHICLE INPUT
    // =================================================

    inputVehicle(&vehicle);


    // =================================================
    // ALGORITHM MENU
    // =================================================

    printf("\n=========================================\n");
    printf("          ALLOCATION METHOD\n");
    printf("=========================================\n");

    printf("\n1. 0/1 Knapsack - Dynamic Programming\n");
    printf("2. Greedy 0/1 Heuristic\n");
    printf("3. Compare Both Algorithms\n");

    printf("\nEnter Choice: ");
    scanf("%d", &choice);


    // =================================================
    // OPTION 1:
    // 0/1 KNAPSACK USING DYNAMIC PROGRAMMING
    // =================================================

    if (choice == 1)
    {
        int dpValue =
            knapsackDP(
                packages,
                n,
                vehicle.capacity,
                selectedDP
            );


        displayAllocation(
            "0/1 KNAPSACK - DYNAMIC PROGRAMMING",
            packages,
            n,
            selectedDP,
            vehicle.capacity,
            dpValue
        );
    }


    // =================================================
    // OPTION 2:
    // GREEDY 0/1 HEURISTIC
    // =================================================

    else if (choice == 2)
    {
        int greedyValue =
            greedyAllocation(
                packages,
                n,
                vehicle.capacity,
                selectedGreedy
            );


        displayAllocation(
            "GREEDY 0/1 ALLOCATION",
            packages,
            n,
            selectedGreedy,
            vehicle.capacity,
            greedyValue
        );
    }


    // =================================================
    // OPTION 3:
    // COMPARE BOTH ALGORITHMS
    // =================================================

    else if (choice == 3)
    {
        // Run Greedy
        int greedyValue =
            greedyAllocation(
                packages,
                n,
                vehicle.capacity,
                selectedGreedy
            );


        // Run Dynamic Programming
        int dpValue =
            knapsackDP(
                packages,
                n,
                vehicle.capacity,
                selectedDP
            );


        // ---------------------------------------------
        // DISPLAY GREEDY RESULT
        // ---------------------------------------------

        displayAllocation(
            "GREEDY 0/1 ALLOCATION",
            packages,
            n,
            selectedGreedy,
            vehicle.capacity,
            greedyValue
        );


        // ---------------------------------------------
        // DISPLAY DP RESULT
        // ---------------------------------------------

        displayAllocation(
            "0/1 KNAPSACK - DYNAMIC PROGRAMMING",
            packages,
            n,
            selectedDP,
            vehicle.capacity,
            dpValue
        );


        // ---------------------------------------------
        // FINAL COMPARISON
        // ---------------------------------------------

        printf("\n=========================================\n");
        printf("          ALGORITHM COMPARISON\n");
        printf("=========================================\n");

        printf("\nGreedy Relief Value : %d\n",
               greedyValue);

        printf("DP Relief Value     : %d\n",
               dpValue);


        // DP performs better
        if (dpValue > greedyValue)
        {
            printf(
                "\nDynamic Programming produced "
                "a better allocation.\n"
            );

            printf(
                "Improvement in Relief Value: %d\n",
                dpValue - greedyValue
            );
        }

        // Both produce same result
        else if (greedyValue == dpValue)
        {
            printf(
                "\nBoth algorithms produced "
                "the same Relief Value.\n"
            );
        }

        // Normally this should not occur because
        // DP gives the optimal solution
        else
        {
            printf(
                "\nGreedy produced a higher "
                "Relief Value.\n"
            );
        }


        printf(
            "\nDynamic Programming guarantees "
            "the optimal solution for the defined "
            "0/1 Knapsack problem.\n"
        );

        printf(
            "The Greedy heuristic does not guarantee "
            "an optimal solution.\n"
        );
    }


    // =================================================
    // INVALID MENU CHOICE
    // =================================================

    else
    {
        printf("\nERROR: Invalid algorithm choice.\n");

        return 1;
    }


    return 0;
}