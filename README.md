# Disaster Relief Supply Distribution Platform (CSEG3053, Project 13)

Design and Analysis of Algorithms, two-phase project.

| Folder | Contents |
|---|---|
| [`Phase_1/`](Phase_1/) | Phase I as presented: 0/1 knapsack (DP) and greedy for one shelter and one vehicle, with proposal, workflow, complexity analysis and test cases |
| [`phase_2(b)/`](phase_2(b)/) | Phase II: Relief Load-and-Route Planner. DP, greedy and branch & bound loading, Dijkstra routing with road damage, merge sort, binary search, benchmarks and 161 automated tests |

Build and run Phase II (GCC, C17):

```
cd "phase_2(b)"
mingw32-make check
mingw32-make run
```

Each folder has a `TIMESTAMP.txt` with its dates. Phase II's full commit history is kept in this repository (`git log`).
