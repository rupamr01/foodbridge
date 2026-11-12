#include "Graph.h"

#include <cmath>
#include <cstddef>
#include <queue>
#include <stack>
#include <vector>

FoodGraph::FoodGraph(int nodes)
    : nodeCount(nodes > 0 ? nodes : 0),
      adjacency(nodes > 0 ? nodes : 0) {}

void FoodGraph::addEdge(int u, int v) {
    if (u < 0 || v < 0 || u >= nodeCount || v >= nodeCount || u == v) {
        return;
    }
    adjacency[u].push_back(v);
    adjacency[v].push_back(u);
}

std::vector<int> FoodGraph::bfs(int start) const {
    std::vector<int> order;
    if (start < 0 || start >= nodeCount) {
        return order;
    }

    std::vector<bool> visited(static_cast<std::size_t>(nodeCount), false);
    std::queue<int> pending;

    visited[start] = true;
    pending.push(start);

    while (!pending.empty()) {
        int current = pending.front();
        pending.pop();
        order.push_back(current);

        for (std::size_t i = 0; i < adjacency[current].size(); ++i) {
            int next = adjacency[current][i];
            if (!visited[next]) {
                visited[next] = true;
                pending.push(next);
            }
        }
    }
    return order;
}

std::vector<int> FoodGraph::dfs(int start) const {
    std::vector<int> order;
    if (start < 0 || start >= nodeCount) {
        return order;
    }

    std::vector<bool> visited(static_cast<std::size_t>(nodeCount), false);
    std::stack<int> pending;
    pending.push(start);

    while (!pending.empty()) {
        int current = pending.top();
        pending.pop();

        if (visited[current]) {
            continue;
        }

        visited[current] = true;
        order.push_back(current);

        // Push in reverse so the first neighbour is visited first.
        for (int i = static_cast<int>(adjacency[current].size()) - 1; i >= 0; --i) {
            int next = adjacency[current][i];
            if (!visited[next]) {
                pending.push(next);
            }
        }
    }
    return order;
}

int FoodGraph::size() const {
    return nodeCount;
}

double degreesToRadians(double degrees) {
    const double PI = 3.14159265358979323846;
    return degrees * PI / 180.0;
}

double distanceKm(double lat1, double lng1, double lat2, double lng2) {
    const double earthRadiusKm = 6371.0;

    double deltaLat = degreesToRadians(lat2 - lat1);
    double deltaLng = degreesToRadians(lng2 - lng1);

    double a = std::sin(deltaLat / 2.0) * std::sin(deltaLat / 2.0)
             + std::cos(degreesToRadians(lat1))
             * std::cos(degreesToRadians(lat2))
             * std::sin(deltaLng / 2.0) * std::sin(deltaLng / 2.0);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return earthRadiusKm * c;
}
