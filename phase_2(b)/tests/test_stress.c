/*
 * Stress tests: the largest supported inputs must finish,
 * stay correct and stay within a time budget.
 */

#include <stdio.h>

#include "benchmark.h"
#include "dataset_io.h"
#include "knapsack_bb.h"
#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "relief_planner.h"
#include "test_util.h"

#define TIME_BUDGET_MS 5000.0

static KnapItem items[MAX_ITEMS];
static int selected[MAX_ITEMS];
static Scenario scenario;
static PlanComparison comparison;

static void testLargeKnapsack(void)
{
    SECTION("n = 1000 random packages");

    int capacity;
    generateDataset(7u, MAX_ITEMS, PROFILE_RANDOM, items, &capacity);
    printf("  capacity W = %d, DP table = %.1f M cells\n",
           capacity, (MAX_ITEMS + 1.0) * (capacity + 1.0) / 1e6);

    double start = timeNowMs();
    int dp = knapsackDP(items, MAX_ITEMS, capacity, selected, NULL);
    double dpMs = timeNowMs() - start;

    start = timeNowMs();
    int greedy = knapsackGreedy(items, MAX_ITEMS, capacity, selected, NULL);
    double greedyMs = timeNowMs() - start;

    start = timeNowMs();
    int bb = knapsackBranchBound(items, MAX_ITEMS, capacity, selected, NULL);
    double bbMs = timeNowMs() - start;

    printf("  DP %.1f ms | Greedy %.2f ms | B&B %.1f ms\n",
           dpMs, greedyMs, bbMs);

    CHECK(dp > 0, "DP completes");
    CHECK(dpMs < TIME_BUDGET_MS, "DP within the time budget");
    CHECK(greedy > 0 && greedy <= dp, "greedy completes, not above optimum");
    CHECK(greedyMs < dpMs, "greedy is faster than DP");
    CHECK(bb == dp, "B&B matches DP on 1000 packages");
    CHECK(bbMs < TIME_BUDGET_MS, "B&B within the time budget");
}

static void testStressScenario(void)
{
    SECTION("Large-data stress scenario");

    char reason[160];

    CHECK(loadScenarioCSV("data/scenario_stress.csv", &scenario,
                          reason, sizeof(reason)) == STATUS_OK,
          "stress scenario loads");
    printf("  %d shelters, %d packages, %d vehicles, %d nodes\n",
           scenario.shelterCount, scenario.packageCount,
           scenario.vehicleCount, scenario.roads.nodeCount);

    double start = timeNowMs();
    comparePlans(&scenario, &comparison);
    double ms = timeNowMs() - start;

    printf("  three full plans in %.1f ms\n", ms);

    for (int a = 0; a < ALGO_COUNT; a++)
        CHECK(comparison.plans[a].status == STATUS_OK,
              "plan completes for every algorithm");

    CHECK(ms < TIME_BUDGET_MS, "planning within the time budget");
    CHECK(comparison.plans[ALGO_DP].totalValue ==
          comparison.plans[ALGO_BRANCH_BOUND].totalValue,
          "exact methods agree on the stress scenario");
}

static void testBranchBoundWorstCase(void)
{
    SECTION("B&B worst case: 200 correlated packages");

    // value = weight + 10 makes every ratio similar, so the bound
    // prunes little and the search tree grows exponentially.
    int capacity;
    generateDataset(11u, 200, PROFILE_CORRELATED, items, &capacity);

    int dp = knapsackDP(items, 200, capacity, selected, NULL);

    double start = timeNowMs();
    int bb = knapsackBranchBound(items, 200, capacity, selected, NULL);
    double ms = timeNowMs() - start;

    printf("  DP = %d | B&B = %s after %.0f ms\n", dp,
           bb == KNAP_ERR_LIMIT ? "node limit reached" : "solved", ms);

    CHECK(bb == dp || bb == KNAP_ERR_LIMIT,
          "B&B is optimal or reports its node limit (no crash)");
    CHECK(ms < TIME_BUDGET_MS, "the node limit stops the search in time");
}

static void testRepeatedRuns(void)
{
    SECTION("Repeated runs (no state leaks)");

    int capacity;
    generateDataset(13u, 300, PROFILE_RANDOM, items, &capacity);

    int first = knapsackBranchBound(items, 300, capacity, selected, NULL);
    int same = 1;

    for (int run = 0; run < 50; run++)
        same &= knapsackBranchBound(items, 300, capacity, selected, NULL) == first;

    CHECK(first == knapsackDP(items, 300, capacity, selected, NULL),
          "B&B solves 300 random packages optimally");
    CHECK(same, "50 repeated runs give the same answer");
}

int main(void)
{
    printf("STRESS TESTS\n");

    testLargeKnapsack();
    testStressScenario();
    testBranchBoundWorstCase();
    testRepeatedRuns();

    return TEST_SUMMARY("test_stress");
}
