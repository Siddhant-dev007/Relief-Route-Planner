#include <stdio.h>
#include <string.h>

#include "benchmark.h"
#include "csv_util.h"
#include "metrics.h"
#include "relief_model.h"
#include "relief_planner.h"

#define BENCH_SEED 2026u

// ---------------------------------------------
// Reproducible pseudo-random numbers (xorshift32)
// ---------------------------------------------

static unsigned nextRandom(unsigned *state)
{
    unsigned x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    return *state = x;
}

static int randomRange(unsigned *state, int low, int high)
{
    return low + (int)(nextRandom(state) % (unsigned)(high - low + 1));
}

const char *profileName(DataProfile profile)
{
    switch (profile)
    {
        case PROFILE_RANDOM:      return "Random";
        case PROFILE_CORRELATED:  return "Correlated";
        case PROFILE_GREEDY_TRAP: return "GreedyTrap";
    }

    return "Unknown";
}

int generateDataset(unsigned seed, int n, DataProfile profile,
                    KnapItem items[], int *capacity)
{
    if (n < 1 || n > MAX_ITEMS || items == NULL || capacity == NULL)
        return 0;

    unsigned state = seed ? seed : 1u;
    long long totalWeight = 0;

    if (profile == PROFILE_GREEDY_TRAP)
    {
        // Capacity 100. Item 0 has the best ratio (100/51) but
        // blocks every pair of 50 kg items worth 90 each:
        // greedy = 100, optimal = 180.
        *capacity = 100;
        items[0] = (KnapItem){ 51, 100 };

        for (int i = 1; i < n; i++)
            items[i] = (KnapItem){ 50, 90 };

        return 1;
    }

    for (int i = 0; i < n; i++)
    {
        int weight = randomRange(&state, 1, 50);
        int value = (profile == PROFILE_CORRELATED)
                        ? weight + 10
                        : randomRange(&state, 1, 100);

        items[i] = (KnapItem){ weight, value };
        totalWeight += weight;
    }

    // Half of the total weight fits: the hardest middle ground.
    *capacity = (int)(totalWeight / 2);
    return 1;
}

// ---------------------------------------------
// Measurement
// ---------------------------------------------

// Checks the selection fits and adds up to the returned value.
static int feasible(const KnapItem items[], int n, int capacity,
                    const int selected[], int value)
{
    long long weight = 0;
    long long total = 0;

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

// Runs one algorithm; repeats fast runs so the timer can see them.
static void measure(Algorithm algorithm, const KnapItem items[], int n,
                    int capacity, BenchRow *row)
{
    static int selected[MAX_ITEMS];
    OpStats stats;

    statsReset(&stats);

    double start = timeNowMs();
    int value = solveKnapsack(algorithm, items, n, capacity,
                              selected, &stats);
    double elapsed = timeNowMs() - start;
    int runs = 1;

    if (value >= 0 && elapsed < 5.0)
    {
        start = timeNowMs();

        while (runs < 200 && timeNowMs() - start < 50.0)
        {
            solveKnapsack(algorithm, items, n, capacity, selected, NULL);
            runs++;
        }

        elapsed += timeNowMs() - start;
    }

    row->algorithm = algorithm;
    row->value = value;
    row->timeMs = elapsed / runs;
    row->operations = statsOperations(&stats);
    row->memoryBytes = stats.peakBytes;

    if (value >= 0)
    {
        row->status = STATUS_OK;
        row->correct = feasible(items, n, capacity, selected, value);
    }
    else
    {
        row->status = (value == KNAP_ERR_LIMIT) ? STATUS_LIMIT_EXCEEDED
                                                : STATUS_INVALID_INPUT;
        row->correct = 0;
    }
}

// Solves one instance with all three algorithms.
static int benchmarkInstance(const char experiment[], DataProfile profile,
                             const KnapItem items[], int n, int capacity,
                             BenchRow rows[], int maxRows)
{
    if (maxRows < ALGO_COUNT)
        return 0;

    for (int a = 0; a < ALGO_COUNT; a++)
    {
        BenchRow *row = &rows[a];

        memset(row, 0, sizeof(*row));
        snprintf(row->experiment, sizeof(row->experiment), "%s", experiment);
        row->profile = profile;
        row->n = n;
        row->capacity = capacity;

        measure((Algorithm)a, items, n, capacity, row);
    }

    // Optimal reference: DP, or branch and bound if DP hit its limit.
    int optimal = -1;

    if (rows[ALGO_DP].status == STATUS_OK)
        optimal = rows[ALGO_DP].value;
    else if (rows[ALGO_BRANCH_BOUND].status == STATUS_OK)
        optimal = rows[ALGO_BRANCH_BOUND].value;

    for (int a = 0; a < ALGO_COUNT; a++)
    {
        BenchRow *row = &rows[a];

        row->optimal = optimal;

        if (row->status != STATUS_OK || optimal <= 0)
            continue;

        row->gapPercent = 100.0 * (optimal - row->value) / optimal;

        // Exact methods must match the optimum; greedy must not exceed it.
        if (a == ALGO_GREEDY)
            row->correct = row->correct && row->value <= optimal;
        else
            row->correct = row->correct && row->value == optimal;
    }

    return ALGO_COUNT;
}

// ---------------------------------------------
// Experiments
// ---------------------------------------------

static KnapItem benchItems[MAX_ITEMS];

int runSizeSweep(BenchRow rows[], int maxRows)
{
    const int sizes[] = { 10, 50, 100, 250, 500, 1000 };
    int added = 0;

    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
    {
        int capacity = 0;

        generateDataset(BENCH_SEED + (unsigned)s, sizes[s],
                        PROFILE_RANDOM, benchItems, &capacity);

        added += benchmarkInstance("SizeSweep", PROFILE_RANDOM, benchItems,
                                   sizes[s], capacity,
                                   rows + added, maxRows - added);
    }

    return added;
}

int runCapacitySweep(BenchRow rows[], int maxRows)
{
    const int capacities[] = { 100, 500, 1000, 2500, 5000 };
    const int n = 200;
    int added = 0;
    int ignored = 0;

    // Same items every time; only the capacity changes.
    generateDataset(BENCH_SEED, n, PROFILE_RANDOM, benchItems, &ignored);

    for (size_t c = 0; c < sizeof(capacities) / sizeof(capacities[0]); c++)
    {
        added += benchmarkInstance("CapacitySweep", PROFILE_RANDOM,
                                   benchItems, n, capacities[c],
                                   rows + added, maxRows - added);
    }

    return added;
}

int runAdversarial(BenchRow rows[], int maxRows)
{
    const int trapSizes[] = { 3, 10, 100 };
    const int correlatedSizes[] = { 20, 30, 40 };
    int added = 0;
    int capacity = 0;

    for (size_t s = 0; s < sizeof(trapSizes) / sizeof(trapSizes[0]); s++)
    {
        generateDataset(BENCH_SEED, trapSizes[s], PROFILE_GREEDY_TRAP,
                        benchItems, &capacity);

        added += benchmarkInstance("Adversarial", PROFILE_GREEDY_TRAP,
                                   benchItems, trapSizes[s], capacity,
                                   rows + added, maxRows - added);
    }

    for (size_t s = 0;
         s < sizeof(correlatedSizes) / sizeof(correlatedSizes[0]); s++)
    {
        generateDataset(BENCH_SEED + (unsigned)s, correlatedSizes[s],
                        PROFILE_CORRELATED, benchItems, &capacity);

        added += benchmarkInstance("Adversarial", PROFILE_CORRELATED,
                                   benchItems, correlatedSizes[s], capacity,
                                   rows + added, maxRows - added);
    }

    return added;
}

int runAllBenchmarks(BenchRow rows[], int maxRows)
{
    int added = runSizeSweep(rows, maxRows);

    added += runCapacitySweep(rows + added, maxRows - added);
    added += runAdversarial(rows + added, maxRows - added);

    return added;
}

Status exportBenchmarkCSV(const BenchRow rows[], int count,
                          const char path[])
{
    FILE *file = csvOpen(path,
        "Experiment,Algorithm,Profile,N,Capacity,Value,Optimal,"
        "GapPercent,TimeMs,Operations,MemoryBytes,Correct,Status");

    if (file == NULL)
        return STATUS_FILE_ERROR;

    for (int i = 0; i < count; i++)
    {
        const BenchRow *r = &rows[i];

        csvWriteRow(file, "%s,%s,%s,%d,%d,%d,%d,%.2f,%.6f,%lld,%zu,%d,%s",
                    r->experiment, algorithmName(r->algorithm),
                    profileName(r->profile), r->n, r->capacity, r->value,
                    r->optimal, r->gapPercent, r->timeMs, r->operations,
                    r->memoryBytes, r->correct, statusText(r->status));
    }

    csvClose(file);
    return STATUS_OK;
}
