#ifndef RELIEF_MODEL_H
#define RELIEF_MODEL_H

#include "common.h"
#include "graph.h"

/*
 * Domain records for one disaster scenario.
 *
 * Relief value of a package = shelter urgency x need severity
 * (unchanged from Phase I).
 */

typedef struct
{
    int id;
    char name[MAX_NAME];
    int population;
    int urgency;        // 1..10
    int node;           // location in the road network
} Shelter;

typedef struct
{
    int id;
    char type[MAX_NAME];
    int weight;         // kg
    int severity;       // need severity 1..10
    int shelterId;      // destination
    int value;          // computed relief value
} Package;

typedef struct
{
    int id;
    int capacity;       // kg
    int available;      // 1 = can be dispatched
} Vehicle;

typedef struct
{
    char name[64];
    int depot;                       // node where vehicles start
    RoadNetwork roads;

    Shelter shelters[MAX_SHELTERS];
    int shelterCount;

    Package packages[MAX_PACKAGES];
    int packageCount;

    Vehicle vehicles[MAX_VEHICLES];
    int vehicleCount;

    // Domain index: shelter IDs in ascending order and the
    // matching array positions, searched with binary search.
    int sortedShelterIds[MAX_SHELTERS];
    int sortedShelterPos[MAX_SHELTERS];
} Scenario;

int computeReliefValue(int urgency, int severity);

// Clears the scenario and sets up an empty road network.
Status scenarioInit(Scenario *scenario, const char name[],
                    int nodeCount, int depot);

Status scenarioAddShelter(Scenario *scenario, const Shelter *shelter);
Status scenarioAddPackage(Scenario *scenario, const Package *package);
Status scenarioAddVehicle(Scenario *scenario, const Vehicle *vehicle);

// Builds the shelter index and computes every package's
// relief value. Call after all records are added.
// Returns STATUS_INVALID_INPUT if a package names an unknown shelter.
Status scenarioFinalize(Scenario *scenario);

// Position of the shelter with this ID, or -1 (binary search).
int findShelter(const Scenario *scenario, int shelterId);

const char *statusText(Status status);
const char *algorithmName(Algorithm algorithm);

#endif
