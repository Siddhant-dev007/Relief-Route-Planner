#include "relief_model.h"
#include "sort_search.h"

int computeReliefValue(int urgency, int severity)
{
    return urgency * severity;
}

Status scenarioInit(Scenario *scenario, const char name[],
                    int nodeCount, int depot)
{
    if (scenario == NULL || name == NULL)
        return STATUS_INVALID_INPUT;

    // Copy scenario name (one place is kept for the '\0')
    int i = 0;

    while (name[i] != '\0' && i < (int)sizeof(scenario->name) - 1)
    {
        scenario->name[i] = name[i];
        i++;
    }

    scenario->name[i] = '\0';

    // Initially there are no shelters, packages or vehicles
    scenario->shelterCount = 0;
    scenario->packageCount = 0;
    scenario->vehicleCount = 0;

    // Initialize road network and check depot
    if (!graphInit(&scenario->roads, nodeCount) ||
        depot < 0 || depot >= nodeCount)
        return STATUS_INVALID_INPUT;

    scenario->depot = depot;

    return STATUS_OK;
}

Status scenarioAddShelter(Scenario *scenario, const Shelter *shelter)
{
    if (scenario->shelterCount >= MAX_SHELTERS)
        return STATUS_LIMIT_EXCEEDED;

    scenario->shelters[scenario->shelterCount++] = *shelter;
    return STATUS_OK;
}

Status scenarioAddPackage(Scenario *scenario, const Package *package)
{
    if (scenario->packageCount >= MAX_PACKAGES)
        return STATUS_LIMIT_EXCEEDED;

    scenario->packages[scenario->packageCount++] = *package;
    return STATUS_OK;
}

Status scenarioAddVehicle(Scenario *scenario, const Vehicle *vehicle)
{
    if (scenario->vehicleCount >= MAX_VEHICLES)
        return STATUS_LIMIT_EXCEEDED;

    scenario->vehicles[scenario->vehicleCount++] = *vehicle;
    return STATUS_OK;
}

static int compareShelterIds(int a, int b, const void *context)
{
    const Scenario *scenario = context;
    int idA = scenario->shelters[a].id;
    int idB = scenario->shelters[b].id;

    return (idA > idB) - (idA < idB);
}

Status scenarioFinalize(Scenario *scenario)
{
    int order[MAX_SHELTERS];

    for (int i = 0; i < scenario->shelterCount; i++)
        order[i] = i;

    if (!mergeSortIndices(order, scenario->shelterCount,
                          compareShelterIds, scenario, NULL))
        return STATUS_NO_MEMORY;

    for (int i = 0; i < scenario->shelterCount; i++)
    {
        scenario->sortedShelterPos[i] = order[i];
        scenario->sortedShelterIds[i] = scenario->shelters[order[i]].id;
    }

    for (int p = 0; p < scenario->packageCount; p++)
    {
        Package *package = &scenario->packages[p];
        int s = findShelter(scenario, package->shelterId);

        if (s < 0)
            return STATUS_INVALID_INPUT;

        package->value = computeReliefValue(
            scenario->shelters[s].urgency, package->severity);
    }

    return STATUS_OK;
}

int findShelter(const Scenario *scenario, int shelterId)
{
    int k = binarySearchInt(scenario->sortedShelterIds,
                            scenario->shelterCount, shelterId, NULL);

    return k < 0 ? -1 : scenario->sortedShelterPos[k];
}

const char *statusText(Status status)
{
    switch (status)
    {
        case STATUS_OK:             return "OK";
        case STATUS_INVALID_INPUT:  return "Invalid input";
        case STATUS_LIMIT_EXCEEDED: return "Size limit exceeded";
        case STATUS_FILE_ERROR:     return "File error";
        case STATUS_NO_MEMORY:      return "Out of memory";
    }

    return "Unknown";
}

const char *algorithmName(Algorithm algorithm)
{
    switch (algorithm)
    {
        case ALGO_DP:           return "Knapsack DP";
        case ALGO_GREEDY:       return "Greedy";
        case ALGO_BRANCH_BOUND: return "Branch & Bound";
        default:                return "Unknown";
    }
}
