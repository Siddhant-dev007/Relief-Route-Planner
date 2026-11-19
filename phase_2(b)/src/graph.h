#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"
#include "metrics.h"

/*
 * Undirected road network stored as an adjacency matrix.
 * A road can be closed (damaged) and reopened without
 * losing its original distance.
 */

#define NO_ROAD    0                    // distance 0 = no road
#define DIST_INF   (1LL << 60)          // "unreachable" distance

typedef struct
{
    int nodeCount;
    int distance[MAX_NODES][MAX_NODES];        // 0 = no road
    unsigned char closed[MAX_NODES][MAX_NODES]; // 1 = damaged
} RoadNetwork;

// Returns 1 on success, 0 if nodeCount is out of range.
int graphInit(RoadNetwork *graph, int nodeCount);

// Adds (or replaces) an undirected road. Returns 1 on success.
int graphAddRoad(RoadNetwork *graph, int from, int to, int distance);

// Marks an existing road closed / open. Returns 1 if the road exists.
int graphCloseRoad(RoadNetwork *graph, int from, int to);
int graphReopenRoad(RoadNetwork *graph, int from, int to);

// Returns 1 if a road exists between the nodes (open or closed).
int graphHasRoad(const RoadNetwork *graph, int from, int to);

// Returns 1 if the road exists and is open.
int graphRoadOpen(const RoadNetwork *graph, int from, int to);

// Single-source shortest paths over open roads.
// dist[v] = DIST_INF if v is unreachable; parent[v] = -1 for none.
// Time O(V^2), auxiliary space O(V). Returns 1 on success.
int dijkstra(
    const RoadNetwork *graph,
    int source,
    long long dist[],
    int parent[],
    OpStats *stats
);

// Writes the node sequence source..target into path.
// Returns the number of nodes, or 0 if the target is unreachable.
int buildPath(
    const int parent[],
    int source,
    int target,
    int path[],
    int maxLength
);

#endif
