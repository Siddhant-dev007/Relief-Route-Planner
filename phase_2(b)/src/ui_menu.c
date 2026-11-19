#include <stdio.h>
#include <string.h>

#include "benchmark.h"
#include "csv_util.h"
#include "dataset_io.h"
#include "relief_planner.h"
#include "ui_io.h"
#include "ui_menu.h"
#include "validation.h"

#define RESULTS_DIR "results"
#define MAX_MANUAL_PACKAGES 50

// ---------------------------------------------
// Application state (one active scenario)
// ---------------------------------------------

static Scenario scenario;
static Scenario loading;          // scratch copy while a file loads
static int scenarioLoaded = 0;

static Plan plan;
static int planReady = 0;
static Algorithm lastAlgorithm = ALGO_DP;

static PlanComparison comparison;
static int comparisonReady = 0;

static BenchRow benchRows[MAX_BENCH_ROWS];
static int benchCount = 0;

static void invalidatePlans(void)
{
    planReady = 0;
    comparisonReady = 0;
}

static int requireScenario(void)
{
    if (!scenarioLoaded)
        printf("\nLoad a scenario first (option 1).\n");

    return scenarioLoaded;
}

// ---------------------------------------------
// Display helpers
// ---------------------------------------------

static void printRoute(const int route[], int length)
{
    if (length == 0)
    {
        printf("-");
        return;
    }

    for (int i = 0; i < length; i++)
        printf(i ? " > %d" : "%d", route[i]);
}

static void printShelters(void)
{
    printf("\n%-4s %-22s %8s %7s %5s\n",
           "ID", "Name", "People", "Urgency", "Node");
    printRule(50);

    for (int i = 0; i < scenario.shelterCount; i++)
    {
        const Shelter *s = &scenario.shelters[i];
        printf("%-4d %-22s %8d %7d %5d\n",
               s->id, s->name, s->population, s->urgency, s->node);
    }
}

static void printPackages(void)
{
    printf("\n%-5s %-16s %7s %8s %8s %6s\n",
           "ID", "Type", "Kg", "Severity", "Shelter", "Value");
    printRule(55);

    for (int i = 0; i < scenario.packageCount; i++)
    {
        const Package *p = &scenario.packages[i];
        printf("%-5d %-16s %7d %8d %8d %6d\n",
               p->id, p->type, p->weight, p->severity,
               p->shelterId, p->value);
    }
}

static void printVehicles(void)
{
    printf("\n%-5s %10s %10s\n", "ID", "Capacity", "Available");
    printRule(27);

    for (int i = 0; i < scenario.vehicleCount; i++)
    {
        const Vehicle *v = &scenario.vehicles[i];
        printf("%-5d %8d kg %10s\n",
               v->id, v->capacity, v->available ? "yes" : "no");
    }
}

static void printRoads(void)
{
    const RoadNetwork *g = &scenario.roads;

    printf("\nDepot: node %d   Nodes: %d\n", scenario.depot, g->nodeCount);
    printf("\n%-6s %-6s %10s %8s\n", "From", "To", "Distance", "State");
    printRule(34);

    for (int u = 0; u < g->nodeCount; u++)
    {
        for (int v = u + 1; v < g->nodeCount; v++)
        {
            if (graphHasRoad(g, u, v))
                printf("%-6d %-6d %7d km %8s\n", u, v, g->distance[u][v],
                       g->closed[u][v] ? "CLOSED" : "open");
        }
    }
}

static const char *shelterName(int id)
{
    int s = findShelter(&scenario, id);
    return s >= 0 ? scenario.shelters[s].name : "?";
}

static void printPlan(const Plan *p)
{
    char memory[32];

    heading("RELIEF OPERATION PLAN");
    printf("Scenario : %s\n", scenario.name);
    printf("Algorithm: %s\n", algorithmName(p->algorithm));

    if (p->status != STATUS_OK)
    {
        printf("\nPlan could not be completed: %s\n", statusText(p->status));
        return;
    }

    printf("\nVehicle assignments\n");
    printf("%-5s %-20s %9s %6s %8s  %s\n",
           "Veh", "Shelter", "Load", "Value", "Dist", "Route");
    printRule(70);

    for (int i = 0; i < p->assignmentCount; i++)
    {
        const Assignment *a = &p->assignments[i];

        printf("%-5d %-20s %3d/%-3dkg %6d %5lldkm  ",
               a->vehicleId, shelterName(a->shelterId), a->loadWeight,
               a->capacity, a->loadValue, a->distance);
        printRoute(a->route, a->routeLength);
        printf("\n      packages:");

        for (int k = 0; k < a->packageCount; k++)
            printf(" %d", a->packageIds[k]);

        printf("\n");
    }

    if (p->assignmentCount == 0)
        printf("No vehicle could be dispatched.\n");

    printf("\nShelter outcomes\n");
    printf("%-4s %-20s %4s %10s %6s %6s %7s\n",
           "ID", "Name", "Urg", "Distance", "Asked", "Got", "Filled");
    printRule(64);

    for (int s = 0; s < p->outcomeCount; s++)
    {
        const ShelterOutcome *o = &p->outcomes[s];
        const Shelter *sh = &scenario.shelters[s];

        if (o->reachable)
            printf("%-4d %-20s %4d %8lldkm %6d %6d %6.1f%%\n",
                   o->shelterId, sh->name, sh->urgency, o->distance,
                   o->requestedValue, o->deliveredValue, o->fulfilment);
        else
            printf("%-4d %-20s %4d %10s %6d %6d %6.1f%%\n",
                   o->shelterId, sh->name, sh->urgency, "UNREACHABLE",
                   o->requestedValue, o->deliveredValue, o->fulfilment);
    }

    formatBytes(p->stats.peakBytes, memory, sizeof(memory));

    printf("\nTotals: value %d | weight %d kg | %d packages sent, %d pending\n",
           p->totalValue, p->totalWeight,
           p->deliveredPackages, p->pendingPackages);
    printf("Fairness: worst shelter %.1f%% | average %.1f%% | "
           "unreachable shelters %d\n",
           p->minFulfilment, p->avgFulfilment, p->unreachableShelters);
    printf("Cost: %.3f ms | %lld operations | peak memory %s\n",
           p->elapsedMs, statsOperations(&p->stats), memory);
}

static void printComparison(const PlanComparison *c)
{
    heading("ALGORITHM COMPARISON (SAME INPUT)");

    int reference = c->plans[ALGO_DP].totalValue;

    printf("%-15s %7s %7s %7s %8s %10s %11s %8s\n",
           "Algorithm", "Value", "Gap%", "Worst%", "Pending",
           "Time(ms)", "Operations", "Memory");
    printRule(80);

    for (int a = 0; a < ALGO_COUNT; a++)
    {
        const Plan *p = &c->plans[a];
        char memory[32];

        if (p->status != STATUS_OK)
        {
            printf("%-15s %s\n", algorithmName(p->algorithm),
                   statusText(p->status));
            continue;
        }

        double gap = reference > 0
                         ? 100.0 * (reference - p->totalValue) / reference
                         : 0.0;

        formatBytes(p->stats.peakBytes, memory, sizeof(memory));

        printf("%-15s %7d %6.1f%% %6.1f%% %8d %10.3f %11lld %8s\n",
               algorithmName(p->algorithm), p->totalValue, gap,
               p->minFulfilment, p->pendingPackages, p->elapsedMs,
               statsOperations(&p->stats), memory);
    }

    printf("\nDP and Branch & Bound give the best load for each vehicle;\n"
           "Greedy may leave value behind (positive gap).\n");
}

static void printBenchRows(const BenchRow rows[], int count)
{
    printf("\n%-13s %-15s %-10s %5s %6s %6s %7s %6s %11s %10s %3s\n",
           "Experiment", "Algorithm", "Profile", "n", "W", "Value",
           "Optimal", "Gap%", "Time(ms)", "Memory", "OK");
    printRule(102);

    for (int i = 0; i < count; i++)
    {
        const BenchRow *r = &rows[i];
        char memory[32];

        formatBytes(r->memoryBytes, memory, sizeof(memory));

        if (r->status != STATUS_OK)
        {
            printf("%-13s %-15s %-10s %5d %6d   -- %s --\n",
                   r->experiment, algorithmName(r->algorithm),
                   profileName(r->profile), r->n, r->capacity,
                   statusText(r->status));
            continue;
        }

        printf("%-13s %-15s %-10s %5d %6d %6d %7d %6.1f %11.4f %10s %3s\n",
               r->experiment, algorithmName(r->algorithm),
               profileName(r->profile), r->n, r->capacity, r->value,
               r->optimal, r->gapPercent, r->timeMs, memory,
               r->correct ? "yes" : "NO");
    }
}

// ---------------------------------------------
// 1. Load scenario
// ---------------------------------------------

static void loadFrom(const char path[])
{
    char reason[160];
    char fallback[160];

    Status status = loadScenarioCSV(path, &loading, reason, sizeof(reason));

    // Allow running from inside build/ or src/ as well.
    if (status == STATUS_FILE_ERROR)
    {
        snprintf(fallback, sizeof(fallback), "../%s", path);
        status = loadScenarioCSV(fallback, &loading, reason, sizeof(reason));

        if (status == STATUS_FILE_ERROR)
            snprintf(reason, sizeof(reason), "Cannot open file '%s'.", path);
    }

    if (status != STATUS_OK)
    {
        printf("\nERROR: %s\n", reason);
        printf("The previous scenario is still active.\n");
        return;
    }

    scenario = loading;
    scenarioLoaded = 1;
    invalidatePlans();

    printf("\nLoaded '%s': %d shelters, %d packages, %d vehicles, %d nodes.\n",
           scenario.name, scenario.shelterCount, scenario.packageCount,
           scenario.vehicleCount, scenario.roads.nodeCount);
}

static void menuLoadScenario(void)
{
    heading("LOAD SCENARIO");
    printf("1. Normal operations\n");
    printf("2. Capacity shock + road closure\n");
    printf("3. Large-data stress\n");
    printf("4. Load from CSV file\n");
    printf("0. Back\n");

    switch (readInt("\nChoice: ", 0, 4))
    {
        case 1: loadFrom("data/scenario_normal.csv"); break;
        case 2: loadFrom("data/scenario_shock.csv");  break;
        case 3: loadFrom("data/scenario_stress.csv"); break;
        case 4:
        {
            char path[100];
            readText("File path (e.g. data/adversarial_greedy.csv): ",
                     path, sizeof(path));
            loadFrom(path);
            break;
        }
        default: break;
    }
}

// ---------------------------------------------
// 2. View scenario
// ---------------------------------------------

static void menuViewScenario(void)
{
    if (!requireScenario())
        return;

    int choice;

    do
    {
        heading("VIEW SCENARIO");
        printf("Scenario: %s\n\n", scenario.name);
        printf("1. Shelters\n2. Packages\n3. Vehicles\n4. Road network\n");
        printf("0. Back\n");

        choice = readInt("\nChoice: ", 0, 4);

        switch (choice)
        {
            case 1: printShelters(); break;
            case 2: printPackages(); break;
            case 3: printVehicles(); break;
            case 4: printRoads();    break;
            default: break;
        }
    } while (choice != 0);
}

// ---------------------------------------------
// 3. Plan operation
// ---------------------------------------------

static void runPlan(Algorithm algorithm)
{
    lastAlgorithm = algorithm;

    planOperation(&scenario, algorithm, &plan);
    planReady = (plan.status == STATUS_OK);
    printPlan(&plan);
}

static void runComparison(void)
{
    comparePlans(&scenario, &comparison);
    comparisonReady = 1;
    printComparison(&comparison);
}

static void menuPlanOperation(void)
{
    if (!requireScenario())
        return;

    heading("PLAN OPERATION");
    printf("1. Knapsack DP (optimal, O(nW))\n");
    printf("2. Greedy (fast, may miss value)\n");
    printf("3. Branch & Bound (optimal, prunes)\n");
    printf("4. Compare all three\n");
    printf("0. Back\n");

    int choice = readInt("\nChoice: ", 0, 4);

    if (choice >= 1 && choice <= 3)
        runPlan((Algorithm)(choice - 1));
    else if (choice == 4)
        runComparison();
}

// ---------------------------------------------
// 4. Routes and road damage
// ---------------------------------------------

static void changeRoad(int close)
{
    int from = readInt("From node: ", 0, scenario.roads.nodeCount - 1);
    int to = readInt("To node  : ", 0, scenario.roads.nodeCount - 1);

    int ok = close ? graphCloseRoad(&scenario.roads, from, to)
                   : graphReopenRoad(&scenario.roads, from, to);

    if (!ok)
    {
        printf("\nThere is no road between %d and %d.\n", from, to);
        return;
    }

    invalidatePlans();
    printf("\nRoad %d-%d is now %s. Choose 'Re-plan' to update routes.\n",
           from, to, close ? "CLOSED" : "open");
}

static void showRoutes(void)
{
    if (!planReady)
    {
        printf("\nNo current plan. Choose 'Re-plan' or option 3.\n");
        return;
    }

    heading("DELIVERY ROUTES");
    printf("Algorithm: %s\n\n", algorithmName(plan.algorithm));

    for (int i = 0; i < plan.assignmentCount; i++)
    {
        const Assignment *a = &plan.assignments[i];

        printf("Vehicle %-4d -> %-20s %5lld km  ",
               a->vehicleId, shelterName(a->shelterId), a->distance);
        printRoute(a->route, a->routeLength);
        printf("\n");
    }

    for (int s = 0; s < plan.outcomeCount; s++)
    {
        if (!plan.outcomes[s].reachable)
            printf("Shelter %d (%s) is UNREACHABLE from the depot.\n",
                   plan.outcomes[s].shelterId, scenario.shelters[s].name);
    }
}

static void menuRoutes(void)
{
    if (!requireScenario())
        return;

    int choice;

    do
    {
        heading("ROUTES & ROAD DAMAGE");
        printf("1. View routes\n");
        printf("2. Close a road\n");
        printf("3. Reopen a road\n");
        printf("4. Re-plan (%s)\n", algorithmName(lastAlgorithm));
        printf("5. View road network\n");
        printf("0. Back\n");

        choice = readInt("\nChoice: ", 0, 5);

        switch (choice)
        {
            case 1: showRoutes();             break;
            case 2: changeRoad(1);            break;
            case 3: changeRoad(0);            break;
            case 4: runPlan(lastAlgorithm);   break;
            case 5: printRoads();             break;
            default: break;
        }
    } while (choice != 0);
}

// ---------------------------------------------
// 5. Compare on current data
// ---------------------------------------------

static void menuCompare(void)
{
    if (requireScenario())
        runComparison();
}

// ---------------------------------------------
// 6. Benchmarks
// ---------------------------------------------

static void saveBenchmarks(void)
{
    ensureDirectory(RESULTS_DIR);

    if (exportBenchmarkCSV(benchRows, benchCount,
                           RESULTS_DIR "/benchmark.csv") == STATUS_OK)
        printf("\nSaved %d rows to " RESULTS_DIR "/benchmark.csv\n",
               benchCount);
    else
        printf("\nERROR: Could not write " RESULTS_DIR "/benchmark.csv\n");
}

static void menuBenchmarks(void)
{
    heading("BENCHMARKS");
    printf("1. Size sweep (n = 10..1000)\n");
    printf("2. Capacity sweep (effect of W on DP)\n");
    printf("3. Adversarial (greedy trap, correlated data)\n");
    printf("4. Run all + export CSV\n");
    printf("0. Back\n");

    int choice = readInt("\nChoice: ", 0, 4);

    if (choice == 0)
        return;

    printf("\nRunning... (the largest cases take a few seconds)\n");

    switch (choice)
    {
        case 1: benchCount = runSizeSweep(benchRows, MAX_BENCH_ROWS);     break;
        case 2: benchCount = runCapacitySweep(benchRows, MAX_BENCH_ROWS); break;
        case 3: benchCount = runAdversarial(benchRows, MAX_BENCH_ROWS);   break;
        case 4: benchCount = runAllBenchmarks(benchRows, MAX_BENCH_ROWS); break;
        default: break;
    }

    printBenchRows(benchRows, benchCount);

    if (choice == 4 || readYesNo("\nExport to CSV? (y/n): "))
        saveBenchmarks();
}

// ---------------------------------------------
// 7. Manual entry (Phase I flow: one shelter, one vehicle)
// ---------------------------------------------

static void printSelection(const char name[], int value,
                           const KnapItem items[], const int selected[],
                           const char types[][MAX_NAME], int n,
                           int capacity, const OpStats *stats, double ms)
{
    int weight = 0;

    printf("\n%s\n", name);
    printRule(40);

    if (value < 0)
    {
        printf("Could not solve: %s\n",
               value == KNAP_ERR_LIMIT ? "instance too large"
                                       : "invalid input");
        return;
    }

    for (int i = 0; i < n; i++)
    {
        if (selected[i])
        {
            printf("  #%-3d %-16s %5d kg  value %d\n",
                   i + 1, types[i], items[i].weight, items[i].value);
            weight += items[i].weight;
        }
    }

    printf("Total value %d | weight %d/%d kg | %.4f ms | %lld ops\n",
           value, weight, capacity, ms, statsOperations(stats));
}

static void menuManualEntry(void)
{
    heading("MANUAL ENTRY");

    KnapItem items[MAX_MANUAL_PACKAGES];
    char types[MAX_MANUAL_PACKAGES][MAX_NAME];
    int selected[MAX_MANUAL_PACKAGES];

    int urgency = readInt("Shelter urgency (1-10): ", MIN_LEVEL, MAX_LEVEL);
    int n = readInt("Number of packages (1-50): ", 1, MAX_MANUAL_PACKAGES);

    for (int i = 0; i < n; i++)
    {
        printf("\nPackage %d\n", i + 1);
        readText("  Supply type        : ", types[i], MAX_NAME);
        items[i].weight = readInt("  Weight (kg, 1-10000): ", 1, 10000);

        int severity = readInt("  Need severity (1-10): ",
                               MIN_LEVEL, MAX_LEVEL);
        items[i].value = computeReliefValue(urgency, severity);
    }

    int capacity = readInt("\nVehicle capacity (kg, 1-100000): ", 1, 100000);

    heading("MANUAL ENTRY RESULTS");

    int values[ALGO_COUNT];

    for (int a = 0; a < ALGO_COUNT; a++)
    {
        OpStats stats;
        statsReset(&stats);

        double start = timeNowMs();
        values[a] = solveKnapsack((Algorithm)a, items, n, capacity,
                                  selected, &stats);
        double ms = timeNowMs() - start;

        printSelection(algorithmName((Algorithm)a), values[a], items,
                       selected, (const char (*)[MAX_NAME])types,
                       n, capacity, &stats, ms);
    }

    if (values[ALGO_DP] > 0 && values[ALGO_GREEDY] >= 0)
        printf("\nGreedy gap vs optimal: %.1f%%\n",
               100.0 * (values[ALGO_DP] - values[ALGO_GREEDY]) /
                   values[ALGO_DP]);
}

// ---------------------------------------------
// 8. Export reports
// ---------------------------------------------

static void exportOne(Status status, const char path[])
{
    if (status == STATUS_OK)
        printf("Saved %s\n", path);
    else
        printf("ERROR: Could not write %s\n", path);
}

static void menuExport(void)
{
    if (!requireScenario())
        return;

    heading("EXPORT REPORTS");
    printf("1. Plan (shelter outcomes)\n");
    printf("2. Routes (vehicle assignments)\n");
    printf("3. Algorithm comparison\n");
    printf("4. Everything\n");
    printf("0. Back\n");

    int choice = readInt("\nChoice: ", 0, 4);

    if (choice == 0)
        return;

    if ((choice == 1 || choice == 2 || choice == 4) && !planReady)
    {
        printf("\nNo current plan; running Knapsack DP first.\n");
        runPlan(ALGO_DP);
    }

    if ((choice == 3 || choice == 4) && !comparisonReady)
    {
        comparePlans(&scenario, &comparison);
        comparisonReady = 1;
    }

    ensureDirectory(RESULTS_DIR);
    printf("\n");

    if (choice == 1 || choice == 4)
        exportOne(exportPlanCSV(&scenario, &plan, RESULTS_DIR "/plan.csv"),
                  RESULTS_DIR "/plan.csv");

    if (choice == 2 || choice == 4)
        exportOne(exportRoutesCSV(&scenario, &plan, RESULTS_DIR "/routes.csv"),
                  RESULTS_DIR "/routes.csv");

    if (choice == 3 || choice == 4)
        exportOne(exportComparisonCSV(&comparison,
                                      RESULTS_DIR "/comparison.csv"),
                  RESULTS_DIR "/comparison.csv");
}

// ---------------------------------------------
// Main menu
// ---------------------------------------------

void runMainMenu(void)
{
    int choice;

    do
    {
        heading("RELIEF LOAD-AND-ROUTE PLANNER");

        if (scenarioLoaded)
            printf("Active scenario: %s\n\n", scenario.name);
        else
            printf("No scenario loaded.\n\n");

        printf("1. Load Scenario\n");
        printf("2. View Scenario\n");
        printf("3. Plan Operation\n");
        printf("4. Routes & Road Damage\n");
        printf("5. Compare Algorithms (current data)\n");
        printf("6. Benchmarks\n");
        printf("7. Manual Entry\n");
        printf("8. Export Reports\n");
        printf("0. Exit\n");

        choice = readInt("\nChoice: ", 0, 8);

        switch (choice)
        {
            case 1: menuLoadScenario();  break;
            case 2: menuViewScenario();  break;
            case 3: menuPlanOperation(); break;
            case 4: menuRoutes();        break;
            case 5: menuCompare();       break;
            case 6: menuBenchmarks();    break;
            case 7: menuManualEntry();   break;
            case 8: menuExport();        break;
            default: break;
        }
    } while (choice != 0);

    printf("\nGoodbye.\n");
}
