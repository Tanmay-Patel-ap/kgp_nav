#include "pathfinder.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include <cmath>
#include <iostream>

using pdi = std::pair<double, int>;

// ── NEAREST NODE ──────────────────────────────────────────────────
static double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // meters
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dlat/2)*std::sin(dlat/2) +
               std::cos(lat1*M_PI/180.0)*std::cos(lat2*M_PI/180.0)*
               std::sin(dlon/2)*std::sin(dlon/2);
    return R * 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
}

int nearestNode(const Graph& graph, double lat, double lon) {
    int    best_id   = -1;
    double best_dist = std::numeric_limits<double>::infinity();

    for (const auto& [id, node] : graph.node_map) {
        double d = haversine(lat, lon, node.lat, node.lon);
        if (d < best_dist) {
            best_dist = d;
            best_id   = id;
        }
    }
    return best_id;
}

RouteResult findRoute(const Graph& graph, int from_id, int to_id) {
    RouteResult result;
    result.found    = false;
    result.distance = 0.0;

    // Validate nodes exist
    if (graph.node_map.find(from_id) == graph.node_map.end()) {
        std::cerr << "[pathfinder] from_id not found: " << from_id << "\n";
        return result;
    }
    if (graph.node_map.find(to_id) == graph.node_map.end()) {
        std::cerr << "[pathfinder] to_id not found: " << to_id << "\n";
        return result;
    }

    // Same node
    if (from_id == to_id) {
        result.found    = true;
        result.distance = 0.0;
        result.node_ids = { from_id };
        return result;
    }

    // ── DIJKSTRA ────────────────────────────────────────────────
    const double INF = std::numeric_limits<double>::infinity();

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int>    prev;

    // Init all known nodes to INF
    for (const auto& [id, _] : graph.node_map) {
        dist[id] = INF;
    }
    dist[from_id] = 0.0;

    std::priority_queue<pdi, std::vector<pdi>, std::greater<pdi>> pq;
    pq.push({ 0.0, from_id });

    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();

        if (d > dist[u]) continue; // stale entry
        if (u == to_id)  break;    // found destination

        auto it = graph.adj.find(u);
        if (it == graph.adj.end()) continue;

        for (const auto& [v, w] : it->second) {
            double nd = dist[u] + w;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.push({ nd, v });
            }
        }
    }

    // No path found
    if (dist[to_id] == INF) {
        std::cerr << "[pathfinder] No path: " << from_id << " → " << to_id << "\n";
        return result;
    }

    // ── RECONSTRUCT PATH ────────────────────────────────────────
    result.found    = true;
    result.distance = dist[to_id];

    std::vector<int> path;
    for (int cur = to_id; cur != from_id; cur = prev[cur]) {
        path.push_back(cur);
    }
    path.push_back(from_id);
    std::reverse(path.begin(), path.end());

    result.node_ids = path;
    return result;
}