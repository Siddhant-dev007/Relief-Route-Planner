#include <math.h>
#include <stdlib.h>

#include "knapsack_bb.h"
#include "knapsack_dp.h"
#include "knapsack_greedy.h"
#include "sort_search.h"

// ---------------------------------------------
// Search-tree node. Nodes live in one growing arena;
// parent links let the best selection be rebuilt.
// ---------------------------------------------

typedef struct
{
    int level;      // items order[0..level-1] are decided
    int value;
    int weight;
    int parent;     // arena index, -1 for the root
    int taken;      // 1 if order[level-1] was included
    double bound;   // optimistic value of any completion
} BBNode;

typedef struct
{
    BBNode *nodes;
    int count;
    int capacity;
} Arena;

typedef struct
{
    int *slots;     // arena indices
    int count;
    int capacity;
} Heap;

typedef struct
{
    const KnapItem *items;
    const int *order;
    int n;
    int limit;      // knapsack capacity
} Problem;

// ---------------------------------------------
// Fractional-knapsack bound from a partial solution.
// ---------------------------------------------

static double upperBound(const Problem *p, int level, int value, int weight)
{
    double bound = value;
    int room = p->limit - weight;

    for (int k = level; k < p->n && room > 0; k++)
    {
        const KnapItem *item = &p->items[p->order[k]];

        if (item->weight <= room)
        {
            room -= item->weight;
            bound += item->value;
        }
        else
        {
            bound += (double)item->value * room / item->weight;
            room = 0;
        }
    }

    return bound;
}

// ---------------------------------------------
// Arena and heap with doubling growth.
// ---------------------------------------------

static int arenaAdd(Arena *arena, BBNode node)
{
    if (arena->count == arena->capacity)
    {
        if (arena->capacity >= MAX_BB_NODES)
            return -1;

        int grown = arena->capacity * 2;

        if (grown > MAX_BB_NODES)
            grown = MAX_BB_NODES;

        BBNode *bigger = realloc(arena->nodes,
                                 (size_t)grown * sizeof(BBNode));

        if (bigger == NULL)
            return -1;

        arena->nodes = bigger;
        arena->capacity = grown;
    }

    arena->nodes[arena->count] = node;
    return arena->count++;
}

static int higher(const Arena *arena, int a, int b)
{
    return arena->nodes[a].bound > arena->nodes[b].bound;
}

static int heapPush(Heap *heap, const Arena *arena, int index)
{
    if (heap->count == heap->capacity)
    {
        int grown = heap->capacity * 2;
        int *bigger = realloc(heap->slots, (size_t)grown * sizeof(int));

        if (bigger == NULL)
            return 0;

        heap->slots = bigger;
        heap->capacity = grown;
    }

    int child = heap->count++;
    heap->slots[child] = index;

    // Sift up while the parent has a smaller bound.
    while (child > 0)
    {
        int parent = (child - 1) / 2;

        if (!higher(arena, heap->slots[child], heap->slots[parent]))
            break;

        int temp = heap->slots[child];
        heap->slots[child] = heap->slots[parent];
        heap->slots[parent] = temp;
        child = parent;
    }

    return 1;
}

static int heapPop(Heap *heap, const Arena *arena)
{
    int top = heap->slots[0];
    heap->slots[0] = heap->slots[--heap->count];

    // Sift down towards the child with the larger bound.
    int parent = 0;

    while (1)
    {
        int left = 2 * parent + 1;
        int right = left + 1;
        int largest = parent;

        if (left < heap->count &&
            higher(arena, heap->slots[left], heap->slots[largest]))
            largest = left;

        if (right < heap->count &&
            higher(arena, heap->slots[right], heap->slots[largest]))
            largest = right;

        if (largest == parent)
            break;

        int temp = heap->slots[parent];
        heap->slots[parent] = heap->slots[largest];
        heap->slots[largest] = temp;
        parent = largest;
    }

    return top;
}

// A node is worth keeping only if its bound can beat best.
// Values are integers, so floor(bound) must exceed best.
static int promising(double bound, int best)
{
    return (long long)floor(bound + 1e-9) > best;
}

// ---------------------------------------------
// Branch and bound search
// ---------------------------------------------

int knapsackBranchBound(
    const KnapItem items[],
    int n,
    int capacity,
    int selected[],
    OpStats *stats
)
{
    if (!knapsackInputValid(items, n, capacity) || selected == NULL)
        return KNAP_ERR_INPUT;

    for (int i = 0; i < n; i++)
        selected[i] = 0;

    if (n == 0 || capacity == 0)
        return 0;

    int *order = malloc((size_t)n * sizeof(int));
    Arena arena = { malloc(1024 * sizeof(BBNode)), 0, 1024 };
    Heap heap = { malloc(1024 * sizeof(int)), 0, 1024 };
    int result = KNAP_ERR_LIMIT;

    if (order == NULL || arena.nodes == NULL || heap.slots == NULL)
        goto cleanup;

    for (int i = 0; i < n; i++)
        order[i] = i;

    if (!mergeSortIndices(order, n, compareItemsByRatio, items, stats))
        goto cleanup;

    Problem problem = { items, order, n, capacity };

    int best = 0;
    int bestNode = -1;      // -1 = take nothing
    size_t peakHeap = 0;

    BBNode root = { 0, 0, 0, -1, 0, upperBound(&problem, 0, 0, 0) };
    int rootIndex = arenaAdd(&arena, root);

    if (rootIndex < 0 || !heapPush(&heap, &arena, rootIndex))
        goto cleanup;

    while (heap.count > 0)
    {
        int current = heapPop(&heap, &arena);
        BBNode node = arena.nodes[current];

        if (stats != NULL)
            stats->nodesExplored++;

        // A leaf has no children; its value was recorded when created.
        if (node.level == n)
            continue;

        // The best value may have improved since this node was queued.
        if (!promising(node.bound, best))
        {
            if (stats != NULL)
                stats->nodesPruned++;
            continue;
        }

        const KnapItem *item = &items[order[node.level]];

        // Child 1: include the next item (only if it fits).
        if (node.weight + item->weight <= capacity)
        {
            BBNode child = {
                node.level + 1,
                node.value + item->value,
                node.weight + item->weight,
                current, 1, 0.0
            };

            child.bound = upperBound(&problem, child.level,
                                     child.value, child.weight);

            int index = arenaAdd(&arena, child);

            if (index < 0)
                goto cleanup;

            if (child.value > best)
            {
                best = child.value;
                bestNode = index;
            }

            if (promising(child.bound, best))
            {
                if (!heapPush(&heap, &arena, index))
                    goto cleanup;
            }
            else if (stats != NULL)
            {
                stats->nodesPruned++;
            }
        }

        // Child 2: exclude the next item.
        BBNode child = {
            node.level + 1, node.value, node.weight,
            current, 0, 0.0
        };

        child.bound = upperBound(&problem, child.level,
                                 child.value, child.weight);

        if (promising(child.bound, best))
        {
            int index = arenaAdd(&arena, child);

            if (index < 0 || !heapPush(&heap, &arena, index))
                goto cleanup;
        }
        else if (stats != NULL)
        {
            stats->nodesPruned++;
        }

        size_t inUse = (size_t)heap.capacity * sizeof(int);

        if (inUse > peakHeap)
            peakHeap = inUse;
    }

    // Rebuild the selection by walking parent links.
    for (int index = bestNode; index > 0; index = arena.nodes[index].parent)
    {
        const BBNode *step = &arena.nodes[index];

        if (step->taken)
            selected[order[step->level - 1]] = 1;
    }

    statsMemory(stats, (size_t)arena.capacity * sizeof(BBNode) +
                       peakHeap + (size_t)n * sizeof(int));

    result = best;

cleanup:
    free(order);
    free(arena.nodes);
    free(heap.slots);
    return result;
}
