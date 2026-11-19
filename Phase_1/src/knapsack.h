#ifndef KNAPSACK_H
#define KNAPSACK_H

#include "relief.h"

int knapsackDP(
    SupplyPackage packages[],
    int n,
    int capacity,
    int selected[]
);

#endif