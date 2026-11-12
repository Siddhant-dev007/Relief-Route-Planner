#ifndef COMMON_H
#define COMMON_H

/*
 * Shared limits, status codes and small types used by every layer.
 * Header only: no functions are defined here.
 */

// ---------------------------------------------
// Capacity limits
// ---------------------------------------------

#define MAX_NODES      100    // road-network vertices (node 0.. MAX-1)
#define MAX_SHELTERS    50
#define MAX_PACKAGES   500    // packages in one scenario
#define MAX_VEHICLES    30
#define MAX_ITEMS     1000    // largest knapsack instance (benchmarks)
#define MAX_NAME        32

#define MIN_LEVEL        1    // urgency and severity range
#define MAX_LEVEL       10

// DP table guard: (n + 1) x (W + 1) int cells, about 100 MB.
#define MAX_DP_CELLS  25000000LL

// Branch-and-bound guard: maximum nodes created in one search.
#define MAX_BB_NODES   2000000

// ---------------------------------------------
// Knapsack return codes (a value >= 0 is the
// best relief value found)
// ---------------------------------------------

#define KNAP_ERR_INPUT  (-1)  // invalid arguments
#define KNAP_ERR_LIMIT  (-2)  // instance too large / out of memory

// ---------------------------------------------
// Status codes
// ---------------------------------------------

typedef enum
{
    STATUS_OK = 0,
    STATUS_INVALID_INPUT,
    STATUS_LIMIT_EXCEEDED,
    STATUS_FILE_ERROR,
    STATUS_NO_MEMORY
} Status;

// ---------------------------------------------
// Loading algorithms that can be swapped fairly
// ---------------------------------------------

typedef enum
{
    ALGO_DP = 0,
    ALGO_GREEDY,
    ALGO_BRANCH_BOUND,
    ALGO_COUNT
} Algorithm;

// One indivisible item for a 0/1 knapsack instance.
typedef struct
{
    int weight;   // kg, must be > 0
    int value;    // relief value, must be >= 0
} KnapItem;

#endif
