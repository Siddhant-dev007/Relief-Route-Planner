#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "csv_util.h"
#include "dataset_io.h"
#include "validation.h"

typedef enum
{
    SECTION_HEADER,
    SECTION_SHELTERS,
    SECTION_PACKAGES,
    SECTION_VEHICLES,
    SECTION_ROADS
} Section;

// Holds roads until NODES is known and the network exists.
typedef struct
{
    int from;
    int to;
    int distance;
    int line;
} PendingRoad;

#define MAX_ROADS (MAX_NODES * 4)

static int startsWithNumber(const char text[])
{
    return isdigit((unsigned char)text[0]) ||
           (text[0] == '-' && isdigit((unsigned char)text[1]));
}

static int parseInts(char *fields[], int first, int count, int out[])
{
    for (int i = 0; i < count; i++)
    {
        if (!csvParseInt(fields[first + i], &out[i]))
            return 0;
    }

    return 1;
}

// Fails the load with a formatted reason including the line number.
#define FAIL_LINE(...)                                         \
    do                                                         \
    {                                                          \
        int used = snprintf(reason, size, "Line %d: ", line);  \
        if (used > 0 && (size_t)used < size)                   \
            snprintf(reason + used, size - (size_t)used,       \
                     __VA_ARGS__);                             \
        fclose(file);                                          \
        return STATUS_INVALID_INPUT;                           \
    } while (0)

Status loadScenarioCSV(const char path[], Scenario *scenario,
                       char reason[], size_t size)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
    {
        snprintf(reason, size, "Cannot open file '%s'.", path);
        return STATUS_FILE_ERROR;
    }

    char name[64] = "Unnamed scenario";
    int nodes = 0;
    int depot = 0;
    int initialised = 0;

    static PendingRoad roads[MAX_ROADS];
    int roadCount = 0;

    Section section = SECTION_HEADER;
    char buffer[CSV_LINE_SIZE];
    char *fields[CSV_MAX_FIELDS];
    int line = 0;
    int status;

    while ((status = csvReadLine(file, buffer, sizeof(buffer))) != 0)
    {
        line++;

        if (status < 0)
            FAIL_LINE("line is too long.");

        if (buffer[0] == '\0' || buffer[0] == '#')
            continue;

        if (buffer[0] == '[')
        {
            if (strncmp(buffer, "[SHELTERS]", 10) == 0)
                section = SECTION_SHELTERS;
            else if (strncmp(buffer, "[PACKAGES]", 10) == 0)
                section = SECTION_PACKAGES;
            else if (strncmp(buffer, "[VEHICLES]", 10) == 0)
                section = SECTION_VEHICLES;
            else if (strncmp(buffer, "[ROADS]", 7) == 0)
                section = SECTION_ROADS;
            else
                FAIL_LINE("unknown section '%s'.", buffer);

            // The network is created once NODES and DEPOT are read.
            if (!initialised)
            {
                if (scenarioInit(scenario, name, nodes, depot) != STATUS_OK)
                    FAIL_LINE("NODES (1..%d) and DEPOT must come first.",
                              MAX_NODES);
                initialised = 1;
            }

            continue;
        }

        int count = csvSplit(buffer, fields, CSV_MAX_FIELDS);
        int v[4];

        switch (section)
        {
            case SECTION_HEADER:
                if (count != 2)
                    FAIL_LINE("expected KEY,value.");

                if (strcmp(fields[0], "NAME") == 0)
                    snprintf(name, sizeof(name), "%s", fields[1]);
                else if (strcmp(fields[0], "NODES") == 0)
                {
                    if (!csvParseInt(fields[1], &nodes))
                        FAIL_LINE("NODES must be a number.");
                }
                else if (strcmp(fields[0], "DEPOT") == 0)
                {
                    if (!csvParseInt(fields[1], &depot))
                        FAIL_LINE("DEPOT must be a number.");
                }
                else
                    FAIL_LINE("unknown key '%s'.", fields[0]);
                break;

            case SECTION_SHELTERS:
            {
                if (!startsWithNumber(fields[0]))
                    break;   // column header

                if (count != 5 || !parseInts(fields, 2, 3, v) ||
                    !csvParseInt(fields[0], &v[3]))
                    FAIL_LINE("shelter needs id,name,population,urgency,node.");

                Shelter s = { v[3], "", v[0], v[1], v[2] };
                snprintf(s.name, sizeof(s.name), "%s", fields[1]);

                if (!validateShelter(&s, nodes))
                    FAIL_LINE("shelter %d has an invalid value.", s.id);

                if (scenarioAddShelter(scenario, &s) != STATUS_OK)
                    FAIL_LINE("more than %d shelters.", MAX_SHELTERS);
                break;
            }

            case SECTION_PACKAGES:
            {
                if (!startsWithNumber(fields[0]))
                    break;

                if (count != 5 || !csvParseInt(fields[0], &v[0]) ||
                    !parseInts(fields, 2, 3, &v[1]))
                    FAIL_LINE("package needs id,type,weight,severity,shelterId.");

                Package p = { v[0], "", v[1], v[2], v[3], 0 };
                snprintf(p.type, sizeof(p.type), "%s", fields[1]);

                if (!validatePackage(&p))
                    FAIL_LINE("package %d has an invalid value.", p.id);

                if (scenarioAddPackage(scenario, &p) != STATUS_OK)
                    FAIL_LINE("more than %d packages.", MAX_PACKAGES);
                break;
            }

            case SECTION_VEHICLES:
            {
                if (!startsWithNumber(fields[0]))
                    break;

                if (count != 3 || !parseInts(fields, 0, 3, v))
                    FAIL_LINE("vehicle needs id,capacity,available.");

                Vehicle veh = { v[0], v[1], v[2] };

                if (!validateVehicle(&veh))
                    FAIL_LINE("vehicle %d has an invalid value.", veh.id);

                if (scenarioAddVehicle(scenario, &veh) != STATUS_OK)
                    FAIL_LINE("more than %d vehicles.", MAX_VEHICLES);
                break;
            }

            case SECTION_ROADS:
            {
                if (!startsWithNumber(fields[0]))
                    break;

                if (count != 3 || !parseInts(fields, 0, 3, v))
                    FAIL_LINE("road needs from,to,distance.");

                if (!validateRoad(v[0], v[1], v[2], nodes))
                    FAIL_LINE("road %d-%d is invalid.", v[0], v[1]);

                if (roadCount >= MAX_ROADS)
                    FAIL_LINE("more than %d roads.", MAX_ROADS);

                roads[roadCount++] = (PendingRoad){ v[0], v[1], v[2], line };
                break;
            }
        }
    }

    fclose(file);

    if (!initialised)
    {
        snprintf(reason, size, "File has no sections.");
        return STATUS_INVALID_INPUT;
    }

    for (int r = 0; r < roadCount; r++)
        graphAddRoad(&scenario->roads, roads[r].from,
                     roads[r].to, roads[r].distance);

    Status result = validateScenario(scenario, reason, size);

    if (result != STATUS_OK)
        return result;

    result = scenarioFinalize(scenario);

    if (result != STATUS_OK)
        snprintf(reason, size, "Could not index shelters.");

    return result;
}

// ---------------------------------------------
// Exports
// ---------------------------------------------

Status exportPlanCSV(const Scenario *scenario, const Plan *plan,
                     const char path[])
{
    FILE *file = csvOpen(path,
        "Algorithm,ShelterID,ShelterName,Urgency,Reachable,DistanceKm,"
        "RequestedValue,DeliveredValue,FulfilmentPercent,PendingPackages");

    if (file == NULL)
        return STATUS_FILE_ERROR;

    for (int s = 0; s < plan->outcomeCount; s++)
    {
        const ShelterOutcome *o = &plan->outcomes[s];
        const Shelter *shelter = &scenario->shelters[s];

        csvWriteRow(file, "%s,%d,%s,%d,%s,%lld,%d,%d,%.1f,%d",
                    algorithmName(plan->algorithm), o->shelterId,
                    shelter->name, shelter->urgency,
                    o->reachable ? "yes" : "no", o->distance,
                    o->requestedValue, o->deliveredValue,
                    o->fulfilment, o->pendingPackages);
    }

    csvClose(file);
    return STATUS_OK;
}

Status exportRoutesCSV(const Scenario *scenario, const Plan *plan,
                       const char path[])
{
    (void)scenario;

    FILE *file = csvOpen(path,
        "Algorithm,VehicleID,Capacity,ShelterID,LoadWeight,LoadValue,"
        "Packages,DistanceKm,Route");

    if (file == NULL)
        return STATUS_FILE_ERROR;

    for (int i = 0; i < plan->assignmentCount; i++)
    {
        const Assignment *a = &plan->assignments[i];
        char packages[1024] = "";
        char route[512] = "";
        size_t used = 0;

        for (int k = 0; k < a->packageCount && used < sizeof(packages); k++)
            used += (size_t)snprintf(packages + used, sizeof(packages) - used,
                                     k ? " %d" : "%d", a->packageIds[k]);

        used = 0;

        for (int k = 0; k < a->routeLength && used < sizeof(route); k++)
            used += (size_t)snprintf(route + used, sizeof(route) - used,
                                     k ? " > %d" : "%d", a->route[k]);

        csvWriteRow(file, "%s,%d,%d,%d,%d,%d,%s,%lld,%s",
                    algorithmName(plan->algorithm), a->vehicleId,
                    a->capacity, a->shelterId, a->loadWeight,
                    a->loadValue, packages, a->distance, route);
    }

    csvClose(file);
    return STATUS_OK;
}

Status exportComparisonCSV(const PlanComparison *comparison,
                           const char path[])
{
    FILE *file = csvOpen(path,
        "Algorithm,Status,TotalValue,TotalWeight,DeliveredPackages,"
        "PendingPackages,MinFulfilment,AvgFulfilment,TimeMs,"
        "Operations,PeakMemoryBytes,GapVsDPPercent");

    if (file == NULL)
        return STATUS_FILE_ERROR;

    int reference = comparison->plans[ALGO_DP].totalValue;

    for (int a = 0; a < ALGO_COUNT; a++)
    {
        const Plan *p = &comparison->plans[a];
        double gap = reference > 0
                         ? 100.0 * (reference - p->totalValue) / reference
                         : 0.0;

        csvWriteRow(file, "%s,%s,%d,%d,%d,%d,%.1f,%.1f,%.4f,%lld,%zu,%.2f",
                    algorithmName(p->algorithm), statusText(p->status),
                    p->totalValue, p->totalWeight, p->deliveredPackages,
                    p->pendingPackages, p->minFulfilment, p->avgFulfilment,
                    p->elapsedMs, statsOperations(&p->stats),
                    p->stats.peakBytes, gap);
    }

    csvClose(file);
    return STATUS_OK;
}
