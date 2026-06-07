#include "graph.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool loadGraph(const std::string& path, Graph& graph) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[graph] Cannot open: " << path << "\n";
        return false;
    }

    json j;
    try {
        f >> j;
    } catch (const json::exception& e) {
        std::cerr << "[graph] JSON parse error: " << e.what() << "\n";
        return false;
    }

    // ── NODES ──────────────────────────────────────────────────
    for (const auto& n : j["nodes"]) {
        Node node;
        node.id     = n["id"];
        node.osm_id = n["osm_id"];
        node.lat    = n["lat"];
        node.lon    = n["lon"];
        node.x      = n["x"];
        node.y      = n["y"];

        graph.nodes.push_back(node);
        graph.node_map[node.id] = node;
    }

    // ── EDGES (bidirectional) ───────────────────────────────────
    for (const auto& e : j["edges"]) {
        Edge edge;
        edge.from   = e["from"];
        edge.to     = e["to"];
        edge.length = e["length"];

        graph.edges.push_back(edge);

        // Add both directions
        graph.adj[edge.from].push_back({ edge.to,   edge.length });
        graph.adj[edge.to  ].push_back({ edge.from, edge.length });
    }

    std::cout << "[graph] Loaded "
              << graph.nodes.size() << " nodes, "
              << graph.edges.size() << " edges\n";
    return true;
}