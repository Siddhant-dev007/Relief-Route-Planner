#include <string.h>

#include "knapsack_bb.h"
#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "relief_planner.h"
#include "sort_search.h"

int solveKnapsack(
    Algorithm algorithm,
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
)
{
    switch (algorithm)
    {
        case ALGO_DP:
            return knapsackDP(items, n, capacity, selected, stats);
        case ALGO_GREEDY:
            return knapsackGreedy(items, n, capacity, selected, stats);
        case ALGO_BRANCH_BOUND:
            return knapsackBranchBound(items, n, capacity, selected, stats);
        default:
            return KNAP_ERR_INPUT;
    }
}

// ---------------------------------------------
// Ordering rules
// ---------------------------------------------

// Higher urgency first, then larger population, then lower ID.
static int compareShelterPriority(int a, int b, const void *context)
{
    const Scenario *scenario = context;
    const Shelter *x = &scenario->shelters[a];
    const Shelter *y = &scenario->shelters[b];

    if (x->urgency != y->urgency)
        return y->urgency - x->urgency;

    if (x->population != y->population)
        return y->population - x->population;

    return x->id - y->id;
}

// Larger capacity first, then lower ID.
static int compareVehicleCapacity(int a, int b, const void *context)
{
    const Scenario *scenario = context;
    const Vehicle *x = &scenario->vehicles[a];
    const Vehicle *y = &scenario->vehicles[b];

    if (x->capacity != y->capacity)
        return y->capacity - x->capacity;

    return x->id - y->id;
}

// ---------------------------------------------
// Helpers
// ---------------------------------------------

// Returns 1 if the shelter has an undelivered package
// light enough for the vehicle.
static int hasLoadFor(const Scenario *scenario, const int delivered[],
                      int shelterId, int capacity)
{
    for (int p = 0; p < scenario->packageCount; p++)
    {
        const Package *package = &scenario->packages[p];

        if (!delivered[p] && package->shelterId == shelterId &&
            package->weight <= capacity)
            return 1;
    }

    return 0;
}

static void summarise(const Scenario *scenario, const int delivered[],
                      Plan *plan)
{
    int counted = 0;
    double sum = 0.0;

    plan->minFulfilment = 100.0;

    for (int p = 0; p < scenario->packageCount; p++)
    {
        const Package *package = &scenario->packages[p];
        ShelterOutcome *outcome =
            &plan->outcomes[findShelter(scenario, package->shelterId)];

        if (delivered[p])
        {
            outcome->deliveredValue += package->value;
            plan->deliveredPackages++;
        }
        else
        {
            outcome->pendingPackages++;
            plan->pendingPackages++;
        }
    }

    for (int s = 0; s < plan->outcomeCount; s++)
    {
        ShelterOutcome *outcome = &plan->outcomes[s];

        if (!outcome->reachable)
            plan->unreachableShelters++;

        if (outcome->requestedValue == 0)
        {
            outcome->fulfilment = 100.0;
            continue;
        }

        outcome->fulfilment = 100.0 * outcome->deliveredValue /
                              outcome->requestedValue;

        if (outcome->fulfilment < plan->minFulfilment)
            plan->minFulfilment = outcome->fulfilment;

        sum += outcome->fulfilment;
        counted++;
    }

    plan->avgFulfilment = counted > 0 ? sum / counted : 100.0;
}

// ---------------------------------------------
// Planning
// ---------------------------------------------

Status planOperation(const Scenario *scenario,
                     Algorithm algorithm, Plan *plan)
{
    if (scenario == NULL || plan == NULL ||
        algorithm < 0 || algorithm >= ALGO_COUNT)
        return STATUS_INVALID_INPUT;

    memset(plan, 0, sizeof(*plan));
    plan->algorithm = algorithm;
    plan->status = STATUS_OK;

    double start = timeNowMs();

    // Step 1: shortest distances from the depot.
    long long dist[MAX_NODES];
    int parent[MAX_NODES];

    if (!dijkstra(&scenario->roads, scenario->depot,
                  dist, parent, &plan->stats))
        return plan->status = STATUS_INVALID_INPUT;

    plan->outcomeCount = scenario->shelterCount;

    for (int s = 0; s < scenario->shelterCount; s++)
    {
        const Shelter *shelter = &scenario->shelters[s];
        ShelterOutcome *outcome = &plan->outcomes[s];

        outcome->shelterId = shelter->id;
        outcome->reachable = dist[shelter->node] < DIST_INF;
        outcome->distance = outcome->reachable ? dist[shelter->node] : -1;
    }

    for (int p = 0; p < scenario->packageCount; p++)
    {
        const Package *package = &scenario->packages[p];
        int s = findShelter(scenario, package->shelterId);

        if (s < 0)
            return plan->status = STATUS_INVALID_INPUT;

        plan->outcomes[s].requestedValue += package->value;
    }

    // Step 2: shelter priority and vehicle order.
    int shelterOrder[MAX_SHELTERS];
    int vehicleOrder[MAX_VEHICLES];

    for (int s = 0; s < scenario->shelterCount; s++)
        shelterOrder[s] = s;

    for (int v = 0; v < scenario->vehicleCount; v++)
        vehicleOrder[v] = v;

    if (!mergeSortIndices(shelterOrder, scenario->shelterCount,
                          compareShelterPriority, scenario, &plan->stats) ||
        !mergeSortIndices(vehicleOrder, scenario->vehicleCount,
                          compareVehicleCapacity, scenario, &plan->stats))
        return plan->status = STATUS_NO_MEMORY;

    // Step 3 and 4: assign each vehicle and choose its load.
    int delivered[MAX_PACKAGES] = {0};
    int visits[MAX_SHELTERS] = {0};  // vehicles sent to each shelter
    KnapItem items[MAX_PACKAGES];
    int packageOf[MAX_PACKAGES];     // item index -> package index
    int selected[MAX_PACKAGES];

    for (int k = 0; k < scenario->vehicleCount; k++)
    {
        const Vehicle *vehicle = &scenario->vehicles[vehicleOrder[k]];

        if (!vehicle->available)
            continue;

        // Fairness rule: among shelters this vehicle can help, pick
        // the one visited least so far; ties go to higher priority.
        int target = -1;

        for (int r = 0; r < scenario->shelterCount; r++)
        {
            int s = shelterOrder[r];

            if (!plan->outcomes[s].reachable ||
                !hasLoadFor(scenario, delivered,
                            scenario->shelters[s].id, vehicle->capacity))
                continue;

            if (target < 0 || visits[s] < visits[target])
                target = s;
        }

        if (target < 0)
            continue;   // nothing left that this vehicle can carry

        visits[target]++;

        const Shelter *shelter = &scenario->shelters[target];
        int count = 0;

        for (int p = 0; p < scenario->packageCount; p++)
        {
            const Package *package = &scenario->packages[p];

            if (!delivered[p] && package->shelterId == shelter->id)
            {
                items[count].weight = package->weight;
                items[count].value = package->value;
                packageOf[count] = p;
                count++;
            }
        }

        OpStats loadStats;
        statsReset(&loadStats);

        int value = solveKnapsack(algorithm, items, count,
                                  vehicle->capacity, selected, &loadStats);

        statsMerge(&plan->stats, &loadStats);

        if (value < 0)
        {
            plan->status = (value == KNAP_ERR_LIMIT)
                               ? STATUS_LIMIT_EXCEEDED
                               : STATUS_INVALID_INPUT;
            return plan->status;
        }

        Assignment *a = &plan->assignments[plan->assignmentCount++];

        a->vehicleId = vehicle->id;
        a->capacity = vehicle->capacity;
        a->shelterId = shelter->id;
        a->loadValue = value;
        a->distance = dist[shelter->node];
        a->routeLength = buildPath(parent, scenario->depot,
                                   shelter->node, a->route, MAX_NODES);

        for (int i = 0; i < count; i++)
        {
            if (!selected[i])
                continue;

            int p = packageOf[i];

            delivered[p] = 1;
            a->packageIds[a->packageCount++] = scenario->packages[p].id;
            a->loadWeight += scenario->packages[p].weight;
        }

        plan->totalValue += a->loadValue;
        plan->totalWeight += a->loadWeight;
        plan->totalDistance += a->distance;
    }

    summarise(scenario, delivered, plan);
    plan->elapsedMs = timeNowMs() - start;

    return STATUS_OK;
}

Status comparePlans(const Scenario *scenario, PlanComparison *comparison)
{
    if (scenario == NULL || comparison == NULL)
        return STATUS_INVALID_INPUT;

    for (int a = 0; a < ALGO_COUNT; a++)
        planOperation(scenario, (Algorithm)a, &comparison->plans[a]);

    return STATUS_OK;
}
