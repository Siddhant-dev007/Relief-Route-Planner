/*
 * Integration tests: scenario file -> plan -> outcomes.
 * Run from the project folder (mingw32-make check does this).
 */

#include <stdio.h>
#include <string.h>

#include "dataset_io.h"
#include "relief_planner.h"
#include "test_util.h"

static Scenario scenario;
static Plan plan;
static PlanComparison comparison;

static int load(const char path[])
{
    char reason[160];
    Status status = loadScenarioCSV(path, &scenario, reason, sizeof(reason));

    if (status != STATUS_OK)
        printf("  could not load %s: %s\n", path, reason);

    return status == STATUS_OK;
}

// Checks the invariants every plan must satisfy.
static void checkPlanInvariants(const Plan *p, const char label[])
{
    int capacityOk = 1, reachableOk = 1, uniqueOk = 1, totalsOk = 1;
    int seen[MAX_PACKAGES] = {0};
    int value = 0, weight = 0;
    char name[160];

    for (int i = 0; i < p->assignmentCount; i++)
    {
        const Assignment *a = &p->assignments[i];

        capacityOk &= a->loadWeight <= a->capacity;
        reachableOk &= a->routeLength > 0;
        value += a->loadValue;
        weight += a->loadWeight;

        for (int k = 0; k < a->packageCount; k++)
        {
            for (int q = 0; q < scenario.packageCount; q++)
            {
                if (scenario.packages[q].id == a->packageIds[k])
                {
                    uniqueOk &= !seen[q];
                    seen[q] = 1;
                    reachableOk &= scenario.packages[q].shelterId == a->shelterId;
                }
            }
        }
    }

    totalsOk = value == p->totalValue && weight == p->totalWeight &&
               p->deliveredPackages + p->pendingPackages ==
                   scenario.packageCount;

    snprintf(name, sizeof(name), "%s: loads within capacity", label);
    CHECK(capacityOk, name);
    snprintf(name, sizeof(name), "%s: every load has a route and the right shelter", label);
    CHECK(reachableOk, name);
    snprintf(name, sizeof(name), "%s: no package is sent twice", label);
    CHECK(uniqueOk, name);
    snprintf(name, sizeof(name), "%s: totals add up", label);
    CHECK(totalsOk, name);
}

static void testScenario(const char path[], const char label[])
{
    char name[160];

    snprintf(name, sizeof(name), "%s loads", label);
    CHECK(load(path), name);

    comparePlans(&scenario, &comparison);

    for (int a = 0; a < ALGO_COUNT; a++)
    {
        const Plan *p = &comparison.plans[a];
        char sub[64];

        snprintf(sub, sizeof(sub), "%s / %s", label, algorithmName(p->algorithm));
        snprintf(name, sizeof(name), "%s: status OK", sub);
        CHECK(p->status == STATUS_OK, name);
        checkPlanInvariants(p, sub);
    }

    // Each DP and B&B load is optimal for the same vehicle + shelter,
    // so the two exact methods must produce identical plans.
    snprintf(name, sizeof(name), "%s: DP and B&B totals agree", label);
    CHECK(comparison.plans[ALGO_DP].totalValue ==
          comparison.plans[ALGO_BRANCH_BOUND].totalValue, name);
}

static void testShockEffects(void)
{
    SECTION("Shock scenario effects");

    CHECK(load("data/scenario_shock.csv"), "shock scenario loads");
    planOperation(&scenario, ALGO_DP, &plan);

    int hilltop = findShelter(&scenario, 2);

    CHECK(hilltop >= 0 && !plan.outcomes[hilltop].reachable,
          "Hilltop Temple is cut off");
    CHECK(plan.unreachableShelters == 1, "exactly one shelter unreachable");
    CHECK(plan.outcomes[hilltop].deliveredValue == 0,
          "nothing is sent to an unreachable shelter");

    int vehicle12Used = 0;

    for (int i = 0; i < plan.assignmentCount; i++)
        vehicle12Used |= plan.assignments[i].vehicleId == 12;

    CHECK(!vehicle12Used, "unavailable vehicle 12 is not dispatched");
}

static void testRerouting(void)
{
    SECTION("Rerouting after a road closure");

    CHECK(load("data/scenario_normal.csv"), "normal scenario loads");
    planOperation(&scenario, ALGO_DP, &plan);

    int stadium = findShelter(&scenario, 3);
    long long before = plan.outcomes[stadium].distance;

    // Stadium (node 6) is reached via 4-6; closing it forces a detour.
    graphCloseRoad(&scenario.roads, 4, 6);
    planOperation(&scenario, ALGO_DP, &plan);
    long long after = plan.outcomes[stadium].distance;

    CHECK(before == 13, "stadium is 13 km before the closure");
    CHECK(after > before && plan.outcomes[stadium].reachable,
          "stadium is still reachable by a longer route");

    graphReopenRoad(&scenario.roads, 4, 6);
    planOperation(&scenario, ALGO_DP, &plan);
    CHECK(plan.outcomes[stadium].distance == before,
          "reopening restores the original distance");
}

static void testFairness(void)
{
    SECTION("Fairness rule");

    CHECK(load("data/scenario_normal.csv"), "normal scenario loads");
    planOperation(&scenario, ALGO_DP, &plan);

    int distinct = 1;

    for (int i = 0; i < plan.assignmentCount; i++)
        for (int j = 0; j < i; j++)
            distinct &= plan.assignments[i].shelterId !=
                        plan.assignments[j].shelterId;

    CHECK(plan.assignmentCount == 4, "all 4 vehicles are dispatched");
    CHECK(distinct, "4 vehicles go to 4 different shelters");
    CHECK(plan.assignments[0].shelterId == 5,
          "largest vehicle serves the top-priority shelter (urgency 10)");
}

static void testAdversarialScenario(void)
{
    SECTION("Adversarial scenario");

    CHECK(load("data/adversarial_greedy.csv"), "adversarial file loads");
    comparePlans(&scenario, &comparison);

    CHECK(comparison.plans[ALGO_DP].totalValue == 180, "DP delivers 180");
    CHECK(comparison.plans[ALGO_GREEDY].totalValue == 100,
          "greedy is trapped at 100");
    CHECK(comparison.plans[ALGO_BRANCH_BOUND].totalValue == 180,
          "Branch & Bound delivers 180");
}

int main(void)
{
    printf("PLANNER INTEGRATION TESTS\n");

    SECTION("All scenarios, all algorithms");
    testScenario("data/scenario_normal.csv", "normal");
    testScenario("data/scenario_shock.csv", "shock");
    testScenario("data/scenario_stress.csv", "stress");

    testShockEffects();
    testRerouting();
    testFairness();
    testAdversarialScenario();

    return TEST_SUMMARY("test_planner");
}
