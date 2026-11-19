#include <stdio.h>

#include "validation.h"

#define MAX_WEIGHT     10000    // kg per package
#define MAX_CAPACITY  100000    // kg per vehicle
#define MAX_DISTANCE  100000    // per road

int validateLevel(int level)
{
    return level >= MIN_LEVEL && level <= MAX_LEVEL;
}

int validateShelter(const Shelter *shelter, int nodeCount)
{
    return shelter->id > 0 &&
           shelter->name[0] != '\0' &&
           shelter->population >= 0 &&
           validateLevel(shelter->urgency) &&
           shelter->node >= 0 && shelter->node < nodeCount;
}

int validatePackage(const Package *package)
{
    return package->id > 0 &&
           package->type[0] != '\0' &&
           package->weight > 0 && package->weight <= MAX_WEIGHT &&
           validateLevel(package->severity) &&
           package->shelterId > 0;
}

int validateVehicle(const Vehicle *vehicle)
{
    return vehicle->id > 0 &&
           vehicle->capacity > 0 && vehicle->capacity <= MAX_CAPACITY &&
           (vehicle->available == 0 || vehicle->available == 1);
}

int validateRoad(int from, int to, int distance, int nodeCount)
{
    return from >= 0 && from < nodeCount &&
           to >= 0 && to < nodeCount &&
           from != to &&
           distance > 0 && distance <= MAX_DISTANCE;
}

// Fails the scenario with a formatted reason.
#define FAIL(...)                              \
    do                                         \
    {                                          \
        snprintf(reason, size, __VA_ARGS__);   \
        return STATUS_INVALID_INPUT;           \
    } while (0)

Status validateScenario(const Scenario *scenario,
                        char reason[], size_t size)
{
    int nodes = scenario->roads.nodeCount;

    if (scenario->shelterCount == 0)
        FAIL("Scenario has no shelters.");

    if (scenario->vehicleCount == 0)
        FAIL("Scenario has no vehicles.");

    if (scenario->depot < 0 || scenario->depot >= nodes)
        FAIL("Depot node %d is outside 0..%d.", scenario->depot, nodes - 1);

    for (int i = 0; i < scenario->shelterCount; i++)
    {
        const Shelter *s = &scenario->shelters[i];

        if (!validateShelter(s, nodes))
            FAIL("Shelter %d has an invalid field.", s->id);

        for (int j = 0; j < i; j++)
            if (scenario->shelters[j].id == s->id)
                FAIL("Duplicate shelter ID %d.", s->id);
    }

    for (int i = 0; i < scenario->packageCount; i++)
    {
        const Package *p = &scenario->packages[i];

        if (!validatePackage(p))
            FAIL("Package %d has an invalid field.", p->id);

        for (int j = 0; j < i; j++)
            if (scenario->packages[j].id == p->id)
                FAIL("Duplicate package ID %d.", p->id);

        int found = 0;

        for (int s = 0; s < scenario->shelterCount; s++)
            if (scenario->shelters[s].id == p->shelterId)
                found = 1;

        if (!found)
            FAIL("Package %d targets unknown shelter %d.",
                 p->id, p->shelterId);
    }

    for (int i = 0; i < scenario->vehicleCount; i++)
    {
        const Vehicle *v = &scenario->vehicles[i];

        if (!validateVehicle(v))
            FAIL("Vehicle %d has an invalid field.", v->id);

        for (int j = 0; j < i; j++)
            if (scenario->vehicles[j].id == v->id)
                FAIL("Duplicate vehicle ID %d.", v->id);
    }

    snprintf(reason, size, "OK");
    return STATUS_OK;
}
