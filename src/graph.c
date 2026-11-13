#include "graph.h"

static int validNode(const RoadNetwork *graph, int node)
{
    return node >= 0 && node < graph->nodeCount;
}

int graphInit(RoadNetwork *graph, int nodeCount)
{
    if (graph == NULL || nodeCount < 1 || nodeCount > MAX_NODES)
        return 0;

    graph->nodeCount = nodeCount;

    for (int i = 0; i < nodeCount; i++)
    {
        for (int j = 0; j < nodeCount; j++)
        {
            graph->distance[i][j] = NO_ROAD;
            graph->closed[i][j] = 0;
        }
    }

    return 1;
}

int graphAddRoad(RoadNetwork *graph, int from, int to, int distance)
{
    if (graph == NULL || !validNode(graph, from) ||
        !validNode(graph, to) || from == to || distance <= 0)
        return 0;

    graph->distance[from][to] = distance;
    graph->distance[to][from] = distance;
    graph->closed[from][to] = 0;
    graph->closed[to][from] = 0;
    return 1;
}

int graphHasRoad(const RoadNetwork *graph, int from, int to)
{
    return graph != NULL && validNode(graph, from) &&
           validNode(graph, to) &&
           graph->distance[from][to] != NO_ROAD;
}

int graphRoadOpen(const RoadNetwork *graph, int from, int to)
{
    return graphHasRoad(graph, from, to) &&
           !graph->closed[from][to];
}

static int setClosed(RoadNetwork *graph, int from, int to, int closed)
{
    if (!graphHasRoad(graph, from, to))
        return 0;

    graph->closed[from][to] = (unsigned char)closed;
    graph->closed[to][from] = (unsigned char)closed;
    return 1;
}

int graphCloseRoad(RoadNetwork *graph, int from, int to)
{
    return setClosed(graph, from, to, 1);
}

int graphReopenRoad(RoadNetwork *graph, int from, int to)
{
    return setClosed(graph, from, to, 0);
}

int dijkstra(
    const RoadNetwork *graph,
    int source,
    long long dist[],
    int parent[],
    OpStats *stats
)
{
    if (graph == NULL || dist == NULL || parent == NULL ||
        !validNode(graph, source))
        return 0;

    int n = graph->nodeCount;
    int done[MAX_NODES] = {0};

    statsMemory(stats, (size_t)n * (sizeof(long long) +
                                    2 * sizeof(int)));

    for (int v = 0; v < n; v++)
    {
        dist[v] = DIST_INF;
        parent[v] = -1;
    }

    dist[source] = 0;

    // Invariant: every node marked done has its final
    // shortest distance (all road lengths are positive).
    for (int round = 0; round < n; round++)
    {
        int u = -1;

        for (int v = 0; v < n; v++)
        {
            if (!done[v] && (u == -1 || dist[v] < dist[u]))
                u = v;
        }

        if (u == -1 || dist[u] == DIST_INF)
            break;   // remaining nodes are unreachable

        done[u] = 1;

        for (int v = 0; v < n; v++)
        {
            if (done[v] || !graphRoadOpen(graph, u, v))
                continue;

            if (stats != NULL)
                stats->relaxations++;

            long long candidate = dist[u] + graph->distance[u][v];

            if (candidate < dist[v])
            {
                dist[v] = candidate;
                parent[v] = u;
            }
        }
    }

    return 1;
}

int buildPath(
    const int parent[],
    int source,
    int target,
    int path[],
    int maxLength
)
{
    int reversed[MAX_NODES];
    int length = 0;
    int node = target;

    while (node != -1 && length < MAX_NODES)
    {
        reversed[length++] = node;

        if (node == source)
            break;

        node = parent[node];
    }

    if (length == 0 || reversed[length - 1] != source ||
        length > maxLength)
        return 0;

    for (int i = 0; i < length; i++)
        path[i] = reversed[length - 1 - i];

    return length;
}
