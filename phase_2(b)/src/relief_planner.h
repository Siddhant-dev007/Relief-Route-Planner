#ifndef RELIEF_PLANNER_H
#define RELIEF_PLANNER_H

#include "common.h"
#include "metrics.h"
#include "relief_model.h"

/*
 * One dispatch round:
 *   1. Dijkstra from the depot finds which shelters are reachable.
 *   2. Shelters are ranked by urgency, then population (merge sort).
 *   3. Vehicles are taken largest first. Each goes to the reachable
 *      shelter (with a package it can carry) that has received the
 *      fewest vehicles so far; ties go to the higher-ranked shelter.
 *      This spreads vehicles before any shelter gets a second one.
 *   4. The chosen algorithm decides that vehicle's load.
 * A vehicle serves one shelter per round.
 */

typedef struct
{
    int vehicleId;
    int capacity;
    int shelterId;
    int loadWeight;
    int loadValue;
    int packageIds[MAX_PACKAGES];
    int packageCount;
    long long distance;
    int route[MAX_NODES];
    int routeLength;
} Assignment;

typedef struct
{
    int shelterId;
    int reachable;
    long long distance;
    int requestedValue;
    int deliveredValue;
    int pendingPackages;
    double fulfilment;      // delivered / requested value, percent
} ShelterOutcome;

typedef struct
{
    Algorithm algorithm;
    Status status;

    Assignment assignments[MAX_VEHICLES];
    int assignmentCount;

    ShelterOutcome outcomes[MAX_SHELTERS];   // same order as scenario
    int outcomeCount;

    int totalValue;
    int totalWeight;
    long long totalDistance;
    int deliveredPackages;
    int pendingPackages;
    int unreachableShelters;
    double minFulfilment;   // fairness: worst-served shelter
    double avgFulfilment;

    double elapsedMs;
    OpStats stats;
} Plan;

typedef struct
{
    Plan plans[ALGO_COUNT];
} PlanComparison;

// Runs one loading algorithm. Same contract as knapsackDP.
int solveKnapsack(
    Algorithm algorithm,
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
);

// Builds a full dispatch plan with the chosen algorithm.
Status planOperation(const Scenario *scenario,
                     Algorithm algorithm, Plan *plan);

// Runs all three algorithms on the same scenario.
Status comparePlans(const Scenario *scenario,
                    PlanComparison *comparison);

#endif
