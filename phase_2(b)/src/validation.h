#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h>

#include "relief_model.h"

/*
 * Range and consistency rules. Single-record checks return 1 if
 * valid. validateScenario also writes a readable reason.
 */

int validateLevel(int level);                // urgency / severity 1..10
int validateShelter(const Shelter *shelter, int nodeCount);
int validatePackage(const Package *package);
int validateVehicle(const Vehicle *vehicle);
int validateRoad(int from, int to, int distance, int nodeCount);

// Checks every record, duplicate IDs, package destinations and
// that the scenario has at least one shelter and one vehicle.
Status validateScenario(const Scenario *scenario,
                        char reason[], size_t size);

#endif
