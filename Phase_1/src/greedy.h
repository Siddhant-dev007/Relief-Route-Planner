#ifndef GREEDY_H
#define GREEDY_H

#include "relief.h"

int greedyAllocation(
    SupplyPackage packages[],
    int n,
    int capacity,
    int selected[]
);

#endif