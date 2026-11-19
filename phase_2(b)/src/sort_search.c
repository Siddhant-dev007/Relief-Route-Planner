#include <stdlib.h>
#include <string.h>

#include "sort_search.h"

// ---------------------------------------------
// Merges indices[left..mid] and indices[mid+1..right]
// (both already sorted) using temp as scratch space.
// Taking from the left run on ties keeps the sort stable.
// ---------------------------------------------

static void merge(
    int indices[],
    int temp[],
    int left,
    int mid,
    int right,
    IndexCompare compare,
    const void *context,
    OpStats *stats
)
{
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right)
    {
        if (stats != NULL)
            stats->comparisons++;

        if (compare(indices[i], indices[j], context) <= 0)
            temp[k++] = indices[i++];
        else
            temp[k++] = indices[j++];
    }

    while (i <= mid)
        temp[k++] = indices[i++];

    while (j <= right)
        temp[k++] = indices[j++];

    memcpy(&indices[left], &temp[left],
           (size_t)(right - left + 1) * sizeof(int));
}

// Recurrence: T(n) = 2T(n/2) + O(n)  =>  T(n) = O(n log n)
static void mergeSortRecursive(
    int indices[],
    int temp[],
    int left,
    int right,
    IndexCompare compare,
    const void *context,
    OpStats *stats
)
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    mergeSortRecursive(indices, temp, left, mid,
                       compare, context, stats);
    mergeSortRecursive(indices, temp, mid + 1, right,
                       compare, context, stats);
    merge(indices, temp, left, mid, right,
          compare, context, stats);
}

int mergeSortIndices(
    int indices[],
    int n,
    IndexCompare compare,
    const void *context,
    OpStats *stats
)
{
    if (indices == NULL || compare == NULL || n < 0)
        return 0;

    if (n < 2)
        return 1;

    int *temp = malloc((size_t)n * sizeof(int));

    if (temp == NULL)
        return 0;

    statsMemory(stats, (size_t)n * sizeof(int));

    mergeSortRecursive(indices, temp, 0, n - 1,
                       compare, context, stats);

    free(temp);
    return 1;
}

int binarySearchInt(
    const int sorted[],
    int n,
    int key,
    OpStats *stats
)
{
    int low = 0;
    int high = n - 1;

    // Invariant: if key is present, it lies in sorted[low..high].
    while (low <= high)
    {
        int mid = low + (high - low) / 2;

        if (stats != NULL)
            stats->comparisons++;

        if (sorted[mid] == key)
            return mid;

        if (sorted[mid] < key)
            low = mid + 1;
        else
            high = mid - 1;
    }

    return -1;
}
