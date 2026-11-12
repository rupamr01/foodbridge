#pragma once

#include <vector>

// Undirected graph of nearby pickup points.
// Every food listing is a vertex. Two listings are connected when their
// pickup points are within a small distance (for example 1.5 km).
class FoodGraph {
private:
    int nodeCount;
    std::vector<std::vector<int> > adjacency;

public:
    explicit FoodGraph(int nodes = 0);

    void addEdge(int u, int v);

    // Breadth-first search using a queue.
    std::vector<int> bfs(int start) const;

    // Depth-first search using a stack.
    std::vector<int> dfs(int start) const;

    int size() const;
};

double degreesToRadians(double degrees);

// Haversine formula: distance between two latitude/longitude points in km.
double distanceKm(double lat1, double lng1, double lat2, double lng2);
