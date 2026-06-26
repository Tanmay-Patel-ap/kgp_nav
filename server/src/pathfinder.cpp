#define _USE_MATH_DEFINES
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

    if (graph.node_map.empty()) {
        std::cerr << "[pathfinder] Empty graph — cannot find nearest node\n";
        return best_id;
    }

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
    int cur = to_id;
    int steps = 0;
    const int MAX_STEPS = graph.node_map.size() + 1;
    while (cur != from_id && steps < MAX_STEPS) {
        path.push_back(cur);
        auto it = prev.find(cur);
        if (it == prev.end()) { std::cerr << "[pathfinder] Path broken at node " << cur << "\n"; break; }
        cur = it->second;
        steps++;
    }
    if (cur == from_id) path.push_back(from_id);
    if (steps >= MAX_STEPS) std::cerr << "[pathfinder] Path reconstruction exceeded max steps\n";

    std::reverse(path.begin(), path.end());

    result.node_ids = path;
    return result;
}

// ── BEARING ─────────────────────────────────────────────────────────
static double bearing(double lat1, double lon1, double lat2, double lon2) {
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double y = std::sin(dlon) * std::cos(lat2 * M_PI / 180.0);
    double x = std::cos(lat1 * M_PI / 180.0) * std::sin(lat2 * M_PI / 180.0) -
               std::sin(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) * std::cos(dlon);
    double brg = std::atan2(y, x) * 180.0 / M_PI;
    return std::fmod(brg + 360.0, 360.0);
}

static double angleDiff(double a, double b) {
    double d = b - a;
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

// ── GENERATE OUTDOOR STEPS ─────────────────────────────────────────
std::vector<OutdoorStep> generateOutdoorSteps(
    const std::vector<int>& node_ids,
    const Graph& graph,
    const std::string& fromName,
    const std::string& toName,
    double toLat, double toLon)
{
    std::vector<OutdoorStep> steps;
    if (node_ids.size() < 2) return steps;

    steps.push_back({"Start at " + fromName, 0.0});

    // Build edge bearings and distances
    struct EdgeInfo { double brg, dist; };
    std::vector<EdgeInfo> edges;
    edges.reserve(node_ids.size() - 1);

    for (size_t i = 0; i + 1 < node_ids.size(); i++) {
        auto it1 = graph.node_map.find(node_ids[i]);
        auto it2 = graph.node_map.find(node_ids[i + 1]);
        if (it1 == graph.node_map.end() || it2 == graph.node_map.end()) continue;

        double d = 0;
        auto ait = graph.adj.find(node_ids[i]);
        if (ait != graph.adj.end()) {
            for (const auto& [v, w] : ait->second) {
                if (v == node_ids[i + 1]) { d = w; break; }
            }
        }
        double brg = bearing(it1->second.lat, it1->second.lon, it2->second.lat, it2->second.lon);
        edges.push_back({brg, d});
    }

    if (edges.empty()) return steps;

    // Segment edges by turn detection
    struct Segment {
        int startIdx, endIdx; // inclusive indices into edges
        double dist;
    };
    std::vector<Segment> segments;
    std::vector<std::string> turnDirs; // one per segment boundary (size = segments.size()-1)

    segments.push_back({0, 0, edges[0].dist});

    for (size_t i = 1; i < edges.size(); i++) {
        double diff = angleDiff(edges[i - 1].brg, edges[i].brg);
        if (std::abs(diff) > 25.0) {
            // Start new segment
            segments.back().endIdx = (int)i - 1;
            segments.push_back({(int)i, (int)i, edges[i].dist});
            turnDirs.push_back(diff > 0 ? "right" : "left");
        } else {
            segments.back().endIdx = (int)i;
            segments.back().dist += edges[i].dist;
        }
    }

    // Build walk + turn steps from segments
    bool firstTurn = true;
    for (size_t i = 0; i < segments.size(); i++) {
        double segDist = std::round(segments[i].dist);
        if (segDist > 0) {
            steps.push_back({"Walk " + std::to_string((int)segDist) + "m", segDist});
        }
        if (i < turnDirs.size()) {
            if (firstTurn) {
                steps.push_back({"Take the " + turnDirs[i] + " road", 0.0});
                firstTurn = false;
            } else {
                std::string dir = turnDirs[i];
                dir[0] = std::toupper(dir[0]);
                steps.push_back({"Turn " + turnDirs[i], 0.0});
            }
        }
    }

    // Final: building position relative to last edge direction
    if (toLat != 0.0 && toLon != 0.0) {
        auto lastIt = graph.node_map.find(node_ids.back());
        if (lastIt != graph.node_map.end()) {
            double lastBrg = edges.empty() ? 0.0 : edges.back().brg;
            double toBrg = bearing(lastIt->second.lat, lastIt->second.lon, toLat, toLon);
            double diff = angleDiff(lastBrg, toBrg);
            std::string pos;
            if (std::abs(diff) < 30.0) {
                pos = "ahead";
            } else {
                pos = (diff > 0) ? "on your right" : "on your left";
            }
            steps.push_back({toName + " is " + pos, 0.0});
        }
    }

    return steps;
}