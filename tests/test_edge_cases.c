/*
 * Boundary, invalid and adversarial inputs.
 * Malformed scenario files are written to results/ and loaded back.
 */

#include <stdio.h>
#include <string.h>

#include "csv_util.h"
#include "dataset_io.h"
#include "knapsack_bb.h"
#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "relief_planner.h"
#include "test_util.h"
#include "validation.h"

static Scenario scenario;
static Plan plan;

// Runs all three algorithms; returns 1 if all give the expected value.
static int allReturn(const KnapItem items[], int n, int capacity, int expected)
{
    int selected[MAX_ITEMS];

    return knapsackDP(items, n, capacity, selected, NULL) == expected &&
           knapsackGreedy(items, n, capacity, selected, NULL) == expected &&
           knapsackBranchBound(items, n, capacity, selected, NULL) == expected;
}

static void testBoundaries(void)
{
    SECTION("Boundary cases");

    KnapItem items[] = { { 10, 50 }, { 20, 60 }, { 30, 90 } };
    KnapItem one[] = { { 40, 70 } };

    CHECK(allReturn(items, 3, 0, 0), "capacity 0 loads nothing");
    CHECK(allReturn(items, 0, 50, 0), "no packages loads nothing");
    CHECK(allReturn(one, 1, 40, 70), "single package that exactly fits");
    CHECK(allReturn(one, 1, 39, 0), "single package 1 kg too heavy");
    CHECK(allReturn(items, 3, 60, 200), "everything fits exactly");
    CHECK(allReturn(items, 3, 9, 0), "every package too heavy");

    KnapItem zeroValue[] = { { 5, 0 }, { 5, 0 } };
    CHECK(allReturn(zeroValue, 2, 10, 0), "zero-value packages");
}

static void testInvalidAlgorithmInput(void)
{
    SECTION("Invalid algorithm input");

    KnapItem bad[] = { { 0, 10 } };
    KnapItem negative[] = { { 5, -1 } };
    int selected[4];

    CHECK(knapsackDP(bad, 1, 10, selected, NULL) == KNAP_ERR_INPUT,
          "DP rejects weight 0");
    CHECK(knapsackGreedy(negative, 1, 10, selected, NULL) == KNAP_ERR_INPUT,
          "greedy rejects negative value");
    CHECK(knapsackBranchBound(bad, 1, 10, selected, NULL) == KNAP_ERR_INPUT,
          "B&B rejects weight 0");
    CHECK(knapsackDP(bad, -1, 10, selected, NULL) == KNAP_ERR_INPUT,
          "negative item count is rejected");
    CHECK(knapsackDP(bad, 1, -5, selected, NULL) == KNAP_ERR_INPUT,
          "negative capacity is rejected");

    static KnapItem big[MAX_ITEMS];
    static int bigSelected[MAX_ITEMS];

    for (int i = 0; i < MAX_ITEMS; i++)
        big[i] = (KnapItem){ 1, 1 };

    CHECK(knapsackDP(big, MAX_ITEMS, 100000, bigSelected, NULL) == KNAP_ERR_LIMIT,
          "DP refuses a table above the memory limit");
}

static void testRecordValidation(void)
{
    SECTION("Record validation");

    Shelter shelter = { 1, "Camp", 100, 5, 2 };
    Package package = { 10, "Food", 20, 7, 1, 0 };
    Vehicle vehicle = { 3, 50, 1 };

    CHECK(validateShelter(&shelter, 5), "valid shelter accepted");
    shelter.urgency = 11;
    CHECK(!validateShelter(&shelter, 5), "urgency 11 rejected");
    shelter.urgency = 0;
    CHECK(!validateShelter(&shelter, 5), "urgency 0 rejected");
    shelter.urgency = 5;
    shelter.node = 5;
    CHECK(!validateShelter(&shelter, 5), "node outside network rejected");
    shelter.node = 2;
    shelter.population = -1;
    CHECK(!validateShelter(&shelter, 5), "negative population rejected");

    CHECK(validatePackage(&package), "valid package accepted");
    package.weight = -3;
    CHECK(!validatePackage(&package), "negative weight rejected");
    package.weight = 20;
    package.severity = 0;
    CHECK(!validatePackage(&package), "severity 0 rejected");

    CHECK(validateVehicle(&vehicle), "valid vehicle accepted");
    vehicle.available = 2;
    CHECK(!validateVehicle(&vehicle), "availability 2 rejected");
    vehicle.available = 1;
    vehicle.capacity = 0;
    CHECK(!validateVehicle(&vehicle), "capacity 0 rejected");

    CHECK(validateRoad(0, 1, 5, 3), "valid road accepted");
    CHECK(!validateRoad(1, 1, 5, 3), "self-loop rejected");
    CHECK(!validateRoad(0, 1, -2, 3), "negative distance rejected");
}

// Writes text to a scratch file and tries to load it.
static Status loadText(const char text[], char reason[], size_t size)
{
    const char *path = "results/edge_case.csv";
    FILE *file = fopen(path, "w");

    if (file == NULL)
        return STATUS_FILE_ERROR;

    fputs(text, file);
    fclose(file);

    return loadScenarioCSV(path, &scenario, reason, size);
}

#define HEADER "NAME,Test\nNODES,3\nDEPOT,0\n"
#define SHELTER "[SHELTERS]\n1,Camp,100,5,1\n"
#define VEHICLE "[VEHICLES]\n1,50,1\n"
#define ROAD "[ROADS]\n0,1,4\n"

static void testMalformedFiles(void)
{
    SECTION("Malformed and invalid scenario files");

    char reason[160];

    ensureDirectory("results");

    CHECK(loadScenarioCSV("data/does_not_exist.csv", &scenario, reason,
                          sizeof(reason)) == STATUS_FILE_ERROR,
          "missing file is reported");

    CHECK(loadText(HEADER SHELTER "[PACKAGES]\n1,Food,20,7,1\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_OK,
          "well-formed file loads");

    CHECK(loadText(HEADER SHELTER "[PACKAGES]\n1,Food,abc,7,1\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_INVALID_INPUT,
          "non-numeric weight rejected");
    CHECK(strstr(reason, "Line 7") != NULL, "error names the line number");

    CHECK(loadText(HEADER SHELTER "[PACKAGES]\n1,Food,20,7\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_INVALID_INPUT,
          "missing column rejected");

    CHECK(loadText(HEADER SHELTER "[PACKAGES]\n1,Food,20,7,9\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_INVALID_INPUT,
          "package for unknown shelter rejected");

    CHECK(loadText(HEADER SHELTER "1,Copy,50,4,2\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_INVALID_INPUT,
          "duplicate shelter ID rejected");

    CHECK(loadText(HEADER SHELTER VEHICLE "[ROADS]\n0,7,4\n",
                   reason, sizeof(reason)) == STATUS_INVALID_INPUT,
          "road to unknown node rejected");

    CHECK(loadText(HEADER SHELTER ROAD, reason, sizeof(reason)) ==
              STATUS_INVALID_INPUT,
          "scenario without vehicles rejected");

    CHECK(loadText("NAME,Test\nNODES,0\n" SHELTER, reason, sizeof(reason)) ==
              STATUS_INVALID_INPUT,
          "zero nodes rejected");

    CHECK(loadText(HEADER "[WAREHOUSES]\n1,2\n", reason, sizeof(reason)) ==
              STATUS_INVALID_INPUT,
          "unknown section rejected");
}

static void testAdversarialPlanning(void)
{
    SECTION("Adversarial planning cases");

    char reason[160];

    // Shelter at node 2 has no road at all.
    CHECK(loadText(HEADER "[SHELTERS]\n1,Near,100,5,1\n2,Island,100,10,2\n"
                   "[PACKAGES]\n1,Food,20,7,1\n2,Water,20,9,2\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_OK,
          "island scenario loads");

    planOperation(&scenario, ALGO_DP, &plan);
    CHECK(plan.unreachableShelters == 1, "island shelter is unreachable");
    CHECK(plan.assignmentCount == 1 && plan.assignments[0].shelterId == 1,
          "vehicle skips the higher-urgency island for a reachable shelter");

    // Every road closed: nothing can move, but nothing crashes.
    graphCloseRoad(&scenario.roads, 0, 1);
    planOperation(&scenario, ALGO_GREEDY, &plan);
    CHECK(plan.status == STATUS_OK && plan.assignmentCount == 0,
          "all roads closed gives an empty plan");
    CHECK(plan.totalValue == 0 && plan.pendingPackages == 2,
          "every package stays pending");

    // Every package heavier than every vehicle.
    CHECK(loadText(HEADER SHELTER "[PACKAGES]\n1,Tents,80,7,1\n" VEHICLE ROAD,
                   reason, sizeof(reason)) == STATUS_OK,
          "oversized package scenario loads");
    planOperation(&scenario, ALGO_BRANCH_BOUND, &plan);
    CHECK(plan.assignmentCount == 0, "no vehicle is sent for a load it cannot carry");
}

int main(void)
{
    printf("EDGE-CASE TESTS\n");

    testBoundaries();
    testInvalidAlgorithmInput();
    testRecordValidation();
    testMalformedFiles();
    testAdversarialPlanning();

    return TEST_SUMMARY("test_edge_cases");
}
