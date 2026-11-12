/*
 * Unit tests for the algorithm layer.
 * Knapsack results are checked against a brute-force oracle
 * that tries every subset (small n only).
 */

#include <stdio.h>

#include "benchmark.h"
#include "graph.h"
#include "knapsack_bb.h"
#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "sort_search.h"
#include "test_util.h"

// Exhaustive oracle: O(2^n * n).
static int bruteForce(const KnapItem items[], int n, int capacity)
{
    int best = 0;

    for (long mask = 0; mask < (1L << n); mask++)
    {
        int weight = 0;
        int value = 0;

        for (int i = 0; i < n; i++)
        {
            if (mask & (1L << i))
            {
                weight += items[i].weight;
                value += items[i].value;
            }
        }

        if (weight <= capacity && value > best)
            best = value;
    }

    return best;
}

static int selectionMatches(const KnapItem items[], int n, int capacity,
                            const int selected[], int value)
{
    int weight = 0;
    int total = 0;

    for (int i = 0; i < n; i++)
    {
        if (selected[i])
        {
            weight += items[i].weight;
            total += items[i].value;
        }
    }

    return weight <= capacity && total == value;
}

static void testKnapsackAgainstOracle(void)
{
    SECTION("Knapsack vs brute-force oracle (n = 1..15, 2 profiles)");

    KnapItem items[MAX_ITEMS];
    int selected[MAX_ITEMS];
    int agreeDP = 1, agreeBB = 1, greedyBounded = 1, feasibleAll = 1;
    int cases = 0;

    for (int profile = 0; profile < 2; profile++)
    {
        for (int n = 1; n <= 15; n++)
        {
            for (unsigned seed = 1; seed <= 6; seed++)
            {
                int capacity;
                generateDataset(seed * 97u + (unsigned)n, n,
                                (DataProfile)profile, items, &capacity);

                int oracle = bruteForce(items, n, capacity);

                int dp = knapsackDP(items, n, capacity, selected, NULL);
                feasibleAll &= selectionMatches(items, n, capacity, selected, dp);
                agreeDP &= (dp == oracle);

                int bb = knapsackBranchBound(items, n, capacity, selected, NULL);
                feasibleAll &= selectionMatches(items, n, capacity, selected, bb);
                agreeBB &= (bb == oracle);

                int gr = knapsackGreedy(items, n, capacity, selected, NULL);
                feasibleAll &= selectionMatches(items, n, capacity, selected, gr);
                greedyBounded &= (gr <= oracle);

                cases++;
            }
        }
    }

    printf("  %d random instances checked\n", cases);
    CHECK(agreeDP, "DP equals brute force on every instance");
    CHECK(agreeBB, "Branch & Bound equals brute force on every instance");
    CHECK(greedyBounded, "Greedy never exceeds the optimum");
    CHECK(feasibleAll, "Every selection fits and matches its value");
}

static void testPhaseOneExample(void)
{
    SECTION("Phase I worked example (greedy trap)");

    // Phase I test case 2: greedy 160, optimal 200.
    KnapItem items[] = { { 10, 60 }, { 20, 100 }, { 30, 120 } };
    int selected[3];

    CHECK(knapsackDP(items, 3, 50, selected, NULL) == 220,
          "DP finds 220 (items 2 + 3)");
    CHECK(selected[0] == 0 && selected[1] == 1 && selected[2] == 1,
          "DP selects items 2 and 3");
    CHECK(knapsackGreedy(items, 3, 50, selected, NULL) == 160,
          "Greedy takes 160 (items 1 + 2)");
    CHECK(knapsackBranchBound(items, 3, 50, selected, NULL) == 220,
          "Branch & Bound finds 220");
}

static void testStatistics(void)
{
    SECTION("Operation counters");

    KnapItem items[] = { { 5, 10 }, { 4, 40 }, { 6, 30 }, { 3, 50 } };
    int selected[4];
    OpStats stats;

    statsReset(&stats);
    knapsackDP(items, 4, 10, selected, &stats);
    CHECK(stats.cellsFilled == 40, "DP fills n x W = 40 cells");
    CHECK(stats.peakBytes == 5 * 11 * sizeof(int),
          "DP memory is (n+1)(W+1) ints");

    statsReset(&stats);
    knapsackBranchBound(items, 4, 10, selected, &stats);
    CHECK(stats.nodesExplored > 0, "B&B counts explored nodes");
}

static int compareInts(int a, int b, const void *context)
{
    const int *values = context;
    return (values[a] > values[b]) - (values[a] < values[b]);
}

static void testSortSearch(void)
{
    SECTION("Merge sort and binary search");

    int values[] = { 42, 7, 19, 7, 88, 3, 56, 19, 0, 23 };
    int order[10];

    for (int i = 0; i < 10; i++)
        order[i] = i;

    CHECK(mergeSortIndices(order, 10, compareInts, values, NULL),
          "merge sort succeeds");

    int sorted = 1;

    for (int i = 1; i < 10; i++)
        sorted &= values[order[i - 1]] <= values[order[i]];

    CHECK(sorted, "output is in ascending order");
    CHECK(order[2] == 1 && order[3] == 3, "equal keys keep input order (stable)");

    int keys[] = { 2, 5, 8, 13, 21, 34, 55 };

    CHECK(binarySearchInt(keys, 7, 2, NULL) == 0, "finds the first key");
    CHECK(binarySearchInt(keys, 7, 55, NULL) == 6, "finds the last key");
    CHECK(binarySearchInt(keys, 7, 13, NULL) == 3, "finds a middle key");
    CHECK(binarySearchInt(keys, 7, 4, NULL) == -1, "reports a missing key");
    CHECK(binarySearchInt(keys, 0, 4, NULL) == -1, "handles an empty array");
}

static void testDijkstra(void)
{
    SECTION("Dijkstra and road closures");

    // 0-1 (4), 0-2 (1), 2-1 (2), 1-3 (5), 2-3 (8); node 4 isolated.
    RoadNetwork g;
    long long dist[MAX_NODES];
    int parent[MAX_NODES];
    int path[MAX_NODES];

    graphInit(&g, 5);
    graphAddRoad(&g, 0, 1, 4);
    graphAddRoad(&g, 0, 2, 1);
    graphAddRoad(&g, 2, 1, 2);
    graphAddRoad(&g, 1, 3, 5);
    graphAddRoad(&g, 2, 3, 8);

    CHECK(dijkstra(&g, 0, dist, parent, NULL), "runs on a valid graph");
    CHECK(dist[1] == 3, "0 -> 1 is 3 via node 2");
    CHECK(dist[3] == 8, "0 -> 3 is 8 via 2, 1");
    CHECK(dist[4] == DIST_INF, "isolated node is unreachable");

    int length = buildPath(parent, 0, 3, path, MAX_NODES);
    CHECK(length == 4 && path[0] == 0 && path[1] == 2 &&
          path[2] == 1 && path[3] == 3, "path is 0 > 2 > 1 > 3");
    CHECK(buildPath(parent, 0, 4, path, MAX_NODES) == 0,
          "no path to an unreachable node");

    graphCloseRoad(&g, 2, 1);
    dijkstra(&g, 0, dist, parent, NULL);
    CHECK(dist[1] == 4 && dist[3] == 9, "closure forces the 0-1 road");

    graphCloseRoad(&g, 0, 1);
    graphCloseRoad(&g, 2, 3);
    dijkstra(&g, 0, dist, parent, NULL);
    CHECK(dist[3] == DIST_INF, "closing all routes cuts node 3 off");

    graphReopenRoad(&g, 2, 1);
    dijkstra(&g, 0, dist, parent, NULL);
    CHECK(dist[3] == 8, "reopening restores the shortest route");

    CHECK(!graphAddRoad(&g, 1, 1, 5), "self-loop is rejected");
    CHECK(!graphAddRoad(&g, 0, 9, 5), "unknown node is rejected");
    CHECK(!graphAddRoad(&g, 0, 4, 0), "zero distance is rejected");
    CHECK(!graphCloseRoad(&g, 0, 4), "closing a missing road fails");
}

int main(void)
{
    printf("ALGORITHM UNIT TESTS\n");

    testKnapsackAgainstOracle();
    testPhaseOneExample();
    testStatistics();
    testSortSearch();
    testDijkstra();

    return TEST_SUMMARY("test_algorithms");
}
