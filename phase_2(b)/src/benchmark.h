#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stddef.h>

#include "common.h"

/*
 * Experiment runner. Every instance is solved by all three loading
 * algorithms on identical input; DP supplies the optimal reference.
 * Datasets come from a seeded generator, so runs are reproducible.
 */

typedef enum
{
    PROFILE_RANDOM,        // independent weights and values
    PROFILE_CORRELATED,    // value = weight + 10 (hard for pruning)
    PROFILE_GREEDY_TRAP    // adversarial: best ratio blocks the optimum
} DataProfile;

typedef struct
{
    char experiment[24];
    Algorithm algorithm;
    DataProfile profile;
    int n;
    int capacity;
    int value;
    int optimal;            // -1 if no exact method finished
    double gapPercent;      // (optimal - value) / optimal
    double timeMs;          // average per run
    long long operations;
    size_t memoryBytes;
    int correct;            // feasible, and optimal where required
    Status status;
} BenchRow;

#define MAX_BENCH_ROWS 96

const char *profileName(DataProfile profile);

// Fills items and capacity. Returns 1 on success.
int generateDataset(unsigned seed, int n, DataProfile profile,
                    KnapItem items[], int *capacity);

// Each run function appends rows and returns how many it added.
int runSizeSweep(BenchRow rows[], int maxRows);
int runCapacitySweep(BenchRow rows[], int maxRows);
int runAdversarial(BenchRow rows[], int maxRows);
int runAllBenchmarks(BenchRow rows[], int maxRows);

Status exportBenchmarkCSV(const BenchRow rows[], int count,
                          const char path[]);

#endif
