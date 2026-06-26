#define _USE_MATH_DEFINES
#include "indoor.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <queue>
#include <limits>
#include <cmath>
#include <sys/stat.h>
#include <dirent.h>

using json = nlohmann::json;

// ── LOAD ALL GRAPHS ───────────────────────────────────────────────
bool loadAllIndoorGraphs(const std::string& directory,
                          std::unordered_map<std::string, IndoorGraph>& out) {

    // Check directory exists
    struct stat st;
    if (stat(directory.c_str(), &st) != 0 || !(st.st_mode & S_IFDIR)) {
        std::cerr << "[indoor] Directory not found: " << directory << "\n";
        return false;
    }

    // Scan directory for *_graph.json files
    std::vector<std::string> candidates;
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name(entry->d_name);
            // Match *_graph.json suffix
            if (name.size() > 11 &&
                name.compare(name.size() - 11, 11, "_graph.json") == 0) {
                candidates.push_back(name);
            }
        }
        closedir(dir);
    }

    if (candidates.empty()) {
        std::cerr << "[indoor] No *_graph.json files found in " << directory << "\n";
        return false;
    }

    for (const auto& fname : candidates) {
        std::string path = directory + "/" + fname;
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[indoor] Failed to open: " << path << "\n";
            continue;
        }

        json j;
        try { f >> j; }
        catch (const json::exception& e) {
            std::cerr << "[indoor] JSON parse error in " << fname << ": " << e.what() << "\n";
            continue;
        }

        IndoorGraph g;
        g.id        = j.value("id", 0);
        g.prefix    = j.value("prefix", "");
        g.name      = j.value("name", "");
        g.totalRooms = j.value("total_rooms", 0);

        if (g.prefix.empty()) { std::cerr << "[indoor] Skipping " << fname << " — no prefix\n"; continue; }

        // ── BLOCKS ──────────────────────────────────────────────
        if (j.contains("blocks") && j["blocks"].is_array()) {
            for (const auto& b : j["blocks"]) {
                IndoorBlockInfo bi;
                bi.name   = b.value("name", "");
                bi.floors = b.value("floors", 0);
                g.blocks.push_back(bi);
            }
        }

        // ── NODES ───────────────────────────────────────────────
        if (j.contains("nodes") && j["nodes"].is_array()) {
            for (const auto& n : j["nodes"]) {
                try {
                    IndoorNode node;
                    node.id       = n.at("id");
                    node.name     = n.value("name", "");
                    node.type     = n.value("type", "room");
                    node.block    = n.value("block", "");
                    node.blockIdx = n.value("blockIdx", 0);
                    node.floor    = n.value("floor", 0);
                    g.nodeIndex[node.id] = (int)g.nodes.size();
                    g.nodes.push_back(node);
                } catch (const json::exception& e) {
                    std::cerr << "[indoor] Skipping bad node in " << fname << ": " << e.what() << "\n";
                }
            }
        }

        // ── EDGES ───────────────────────────────────────────────
        if (j.contains("edges") && j["edges"].is_array()) {
            for (const auto& e : j["edges"]) {
                try {
                    IndoorEdge edge;
                    edge.from   = e.at("from");
                    edge.to     = e.at("to");
                    edge.weight = e.value("weight", 1.0);
                    edge.type   = "corridor";

                    // Validate node references
                    if (g.nodeIndex.find(edge.from) == g.nodeIndex.end()) {
                        std::cerr << "[indoor] Edge '" << edge.from << "→" << edge.to
                                  << "' in " << fname << ": from node not found, skipping\n";
                        continue;
                    }
                    if (g.nodeIndex.find(edge.to) == g.nodeIndex.end()) {
                        std::cerr << "[indoor] Edge '" << edge.from << "→" << edge.to
                                  << "' in " << fname << ": to node not found, skipping\n";
                        continue;
                    }

                    g.edges.push_back(edge);
                    g.adj[edge.from].push_back({ edge.to, edge.weight });
                    g.adj[edge.to].push_back({ edge.from, edge.weight });
                } catch (const json::exception& e) {
                    std::cerr << "[indoor] Skipping bad edge in " << fname << ": " << e.what() << "\n";
                }
            }
        }

        // ── CONNECTIONS (staircases/elevators between levels) ────
        if (j.contains("connections") && j["connections"].is_array()) {
            for (const auto& c : j["connections"]) {
                try {
                    IndoorConnection con;
                    con.type     = c.value("type", "staircase");
                    con.fromLevel = c.value("fromLevel", 0);
                    con.fromNode = c.at("fromNode");
                    con.toLevel  = c.value("toLevel", 0);
                    con.toNode   = c.at("toNode");
                    con.weight   = c.value("weight", 1.0);

                    // Validate node references
                    if (g.nodeIndex.find(con.fromNode) == g.nodeIndex.end()) {
                        std::cerr << "[indoor] Connection fromNode '" << con.fromNode
                                  << "' in " << fname << " not found, skipping\n";
                        continue;
                    }
                    if (g.nodeIndex.find(con.toNode) == g.nodeIndex.end()) {
                        std::cerr << "[indoor] Connection toNode '" << con.toNode
                                  << "' in " << fname << " not found, skipping\n";
                        continue;
                    }

                    g.connections.push_back(con);
                    g.adj[con.fromNode].push_back({ con.toNode, con.weight });
                    g.adj[con.toNode].push_back({ con.fromNode, con.weight });
                } catch (const json::exception& e) {
                    std::cerr << "[indoor] Skipping bad connection in " << fname << ": " << e.what() << "\n";
                }
            }
        }

        // ── BACKWARD COMPAT ──────────────────────────────────────
        // Old-format graphs embed staircases/elevators in edges[] with `type` field.
        // If no dedicated connections[] array, extract them from edges.
        if (!j.contains("connections") || !j["connections"].is_array()) {
            for (const auto& e : j["edges"]) {
                try {
                    std::string etype = e.value("type", "");
                    if (etype != "staircase" && etype != "elevator") continue;
                    IndoorConnection con;
                    con.type     = etype;
                    con.fromNode = e.at("from");
                    con.toNode   = e.at("to");
                    con.weight   = e.value("weight", 1.0);
                    // Derive levels from node IDs (BBFRRR scheme)
                    std::string fn = con.fromNode;
                    std::string tn = con.toNode;
                    if (fn.size() >= 3) con.fromLevel = std::stoi(fn.substr(2,1));
                    if (tn.size() >= 3) con.toLevel   = std::stoi(tn.substr(2,1));
                    if (g.nodeIndex.find(con.fromNode) != g.nodeIndex.end() &&
                        g.nodeIndex.find(con.toNode) != g.nodeIndex.end()) {
                        g.connections.push_back(con);
                    }
                } catch (...) {}
            }
            if (g.connections.size() > 0) {
                std::cout << "[indoor] Backward compat: extracted " << g.connections.size()
                          << " connections from edges in " << fname << "\n";
            }
        }

        // Validate no orphan nodes in adjacency (node in adj but not in nodeIndex)
        for (const auto& [id, _] : g.adj) {
            if (g.nodeIndex.find(id) == g.nodeIndex.end()) {
                std::cerr << "[indoor] Orphan adjacency node '" << id
                          << "' in " << fname << " — node missing from nodes list\n";
            }
        }

        out[g.prefix] = g;
        std::cout << "[indoor] Loaded " << g.prefix << " (" << g.nodes.size() << " nodes, "
                  << g.edges.size() << " edges, " << g.connections.size() << " connections)\n";
    }

    std::cout << "[indoor] Loaded " << out.size() << " building"
              << (out.size() == 1 ? "" : "s") << "\n";
    return !out.empty();
}

// ── NAVIGATE INDOORS ──────────────────────────────────────────────
IndoorRouteResult navigateIndoors(const IndoorGraph& graph,
                                   const std::string& fromId,
                                   const std::string& toId) {
    IndoorRouteResult result;
    result.found    = false;
    result.distance = 0.0;

    // Validate nodes exist
    if (graph.nodeIndex.find(fromId) == graph.nodeIndex.end()) {
        std::cerr << "[indoor] fromId not found: " << fromId << "\n";
        return result;
    }
    if (graph.nodeIndex.find(toId) == graph.nodeIndex.end()) {
        std::cerr << "[indoor] toId not found: " << toId << "\n";
        return result;
    }

    // Same node
    if (fromId == toId) {
        result.found    = true;
        result.distance = 0.0;
        result.nodeIds  = { fromId };
        result.steps    = { { "Already at destination", 0.0 } };
        return result;
    }

    // ── DIJKSTRA ────────────────────────────────────────────────
    const double INF = std::numeric_limits<double>::infinity();
    std::unordered_map<std::string, double> dist;
    std::unordered_map<std::string, std::string> prev;

    for (const auto& [id, _] : graph.nodeIndex) {
        dist[id] = INF;
    }
    dist[fromId] = 0.0;

    // Simple priority queue using vector + push_heap
    using Pair = std::pair<double, std::string>;
    std::vector<Pair> pq;
    pq.push_back({ 0.0, fromId });
    std::push_heap(pq.begin(), pq.end(), std::greater<Pair>());

    while (!pq.empty()) {
        std::pop_heap(pq.begin(), pq.end(), std::greater<Pair>());
        auto [d, u] = pq.back(); pq.pop_back();

        if (d > dist[u]) continue;
        if (u == toId)  break;

        auto it = graph.adj.find(u);
        if (it == graph.adj.end()) {
            std::cerr << "[indoor] Node '" << u << "' has no adjacency entries\n";
            continue;
        }

        for (const auto& [v, w] : it->second) {
            double nd = dist[u] + w;
            if (nd < dist[v]) {
                dist[v] = nd;
                prev[v] = u;
                pq.push_back({ nd, v });
                std::push_heap(pq.begin(), pq.end(), std::greater<Pair>());
            }
        }
    }

    std::cout << "[indoor] Dijkstra finished: " << fromId << " → " << toId
              << " distance=" << (dist[toId] == INF ? -1.0 : dist[toId]) << "\n";

    if (dist[toId] == INF) {
        std::cerr << "[indoor] No path: " << fromId << " → " << toId << "\n";
        return result;
    }

    // ── RECONSTRUCT PATH ────────────────────────────────────────
    result.found    = true;
    result.distance = dist[toId];

    std::vector<std::string> path;
    std::string cur = toId;
    int steps = 0;
    const int MAX_STEPS = graph.nodes.size() + 1;
    while (cur != fromId && steps < MAX_STEPS) {
        path.push_back(cur);
        auto it = prev.find(cur);
        if (it == prev.end()) { break; }
        cur = it->second;
        steps++;
    }
    if (cur == fromId) path.push_back(fromId);
    if (steps >= MAX_STEPS) std::cerr << "[indoor] Path reconstruction exceeded max steps\n";
    std::reverse(path.begin(), path.end());
    result.nodeIds = path;

    // ── GENERATE DIRECTIONS ─────────────────────────────────────
    // Helper to trim trailing whitespace from node names
    auto trimR = [](std::string s) -> std::string {
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
        return s;
    };

    auto nodeName = [&](const std::string& id) -> std::string {
        auto it = graph.nodeIndex.find(id);
        if (it == graph.nodeIndex.end()) return id;
        return trimR(graph.nodes[it->second].name);
    };

    auto getNode = [&](const std::string& id) -> const IndoorNode* {
        auto it = graph.nodeIndex.find(id);
        if (it == graph.nodeIndex.end()) return nullptr;
        return &graph.nodes[it->second];
    };

    auto formatFloor = [](int f) -> std::string {
        if (f == 0) return "Ground Floor";
        if (f == 1) return "Floor 1";
        return "Floor " + std::to_string(f);
    };

    auto formatBlockFloor = [&](const IndoorNode* n) -> std::string {
        if (!n) return "";
        return trimR(n->block) + ", " + formatFloor(n->floor);
    };

    const auto* firstNode = getNode(path[0]);
    std::string startDesc = (firstNode ? trimR(firstNode->name) : path[0]);
    if (firstNode) startDesc += " (" + formatBlockFloor(firstNode) + ")";

    // ── New approach: find staircase runs, show walk→preNode → staircase → near postNode → walk→dest ─

    // Helper: sum corridor edge weights between two path indices
    auto sumCorridorWalk = [&](int fromIdx, int toIdx) -> double {
        double total = 0.0;
        for (int i = fromIdx + 1; i <= toIdx; i++) {
            double ew = 0.0;
            auto ait = graph.adj.find(path[i-1]);
            if (ait != graph.adj.end()) {
                for (const auto& [v, w] : ait->second) {
                    if (v == path[i]) { ew = w; break; }
                }
            }
            total += ew;
        }
        return total;
    };

    // Find staircase runs (consecutive level-change edges) in the path
    struct StairRun {
        int preIdx  = -1;   // path index of node before the first staircase edge
        int postIdx = -1;   // path index of node after the last staircase edge
        double totalWeight = 0.0;
        std::string connType = "staircase";
    };
    std::vector<StairRun> runs;

    for (size_t i = 1; i < path.size(); i++) {
        const auto* n1 = getNode(path[i-1]);
        const auto* n2 = getNode(path[i]);
        bool isLC = n1 && n2 && (n1->blockIdx != n2->blockIdx || n1->floor != n2->floor);
        if (!isLC) continue;

        double ew = 0.0;
        auto ait = graph.adj.find(path[i-1]);
        if (ait != graph.adj.end()) {
            for (const auto& [v, w] : ait->second) {
                if (v == path[i]) { ew = w; break; }
            }
        }

        // New run or extend last?
        if (runs.empty() || runs.back().postIdx != (int)i - 1) {
            StairRun r;
            r.preIdx = (int)i - 1;
            r.connType = "staircase";
            for (const auto& c : graph.connections) {
                if ((c.fromNode == path[i-1] && c.toNode == path[i]) ||
                    (c.fromNode == path[i] && c.toNode == path[i-1])) {
                    r.connType = c.type; break;
                }
            }
            runs.push_back(r);
        }
        runs.back().postIdx = (int)i;
        runs.back().totalWeight += ew;
    }

    // ── Generate steps ─────────────────────────────────────────────
    result.steps.push_back({ "Start at " + startDesc, 0.0 });

    int curIdx = 0;

    for (size_t ri = 0; ri < runs.size(); ri++) {
        const auto& run = runs[ri];

        // Walk from curIdx to preNode
        double wd = sumCorridorWalk(curIdx, run.preIdx);
        if (wd > 0.0) {
            const auto* pn = getNode(path[run.preIdx]);
            result.steps.push_back({ "Walk " + std::to_string((int)std::round(wd)) + "m → " + (pn ? trimR(pn->name) : path[run.preIdx]), wd });
        }

        // Take staircase / corridor
        const auto* pre = getNode(path[run.preIdx]);
        const auto* post = getNode(path[run.postIdx]);
        bool floorChanged = pre && post && pre->floor != post->floor;
        // Override to "corridor" for same-floor cross-block edges
        std::string label = run.connType;
        if (pre && post && pre->floor == post->floor && pre->blockIdx != post->blockIdx) {
            label = "corridor";
        }
        std::string takeMsg;
        if (floorChanged) {
            takeMsg = "Take " + label + " from " + (pre ? formatFloor(pre->floor) : "")
                    + " to " + (post ? formatFloor(post->floor) : "");
        } else {
            takeMsg = "Take " + label + " from " + (pre ? trimR(pre->block) : "")
                    + " to " + (post ? trimR(post->block) : "");
        }
        result.steps.push_back({ takeMsg, run.totalWeight });

        // "You are near" the second closest point (unless it's the destination)
        if (post && run.postIdx != (int)path.size() - 1) {
            result.steps.push_back({ "You are near " + trimR(post->name), 0.0 });
        }

        curIdx = run.postIdx;
    }

    // Final walk to destination
    double fw = sumCorridorWalk(curIdx, (int)path.size() - 1);
    if (fw > 0.0) {
        const auto* dest = getNode(path.back());
        result.steps.push_back({ "Walk " + std::to_string((int)std::round(fw)) + "m → " + (dest ? trimR(dest->name) : path.back()), fw });
    }

    // Arrive at destination
    const auto* lastNode = getNode(path.back());
    if (lastNode && path.size() > 1) {
        result.steps.push_back({ "Arrive at " + trimR(lastNode->name) + " (" + formatBlockFloor(lastNode) + ")", 0.0 });
    }

    return result;
}
