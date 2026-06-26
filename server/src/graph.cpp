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

    if (!j.contains("nodes") || !j["nodes"].is_array()) {
        std::cerr << "[graph] Missing or invalid 'nodes' array\n";
        return false;
    }
    if (!j.contains("edges") || !j["edges"].is_array()) {
        std::cerr << "[graph] Missing or invalid 'edges' array\n";
        return false;
    }

    // ── NODES ──────────────────────────────────────────────────
    for (const auto& n : j["nodes"]) {
        if (!n.is_object()) { std::cerr << "[graph] Skipping non-object node\n"; continue; }
        try {
            Node node;
            node.id     = n.at("id");
            node.osm_id = n.value("osm_id", -1L);
            node.lat    = n.at("lat");
            node.lon    = n.at("lon");
            node.x      = n.value("x", 0.0);
            node.y      = n.value("y", 0.0);

            graph.nodes.push_back(node);
            graph.node_map[node.id] = node;
        } catch (const json::exception& e) {
            std::cerr << "[graph] Skipping bad node: " << e.what() << "\n";
        }
    }

    // ── EDGES (bidirectional) ───────────────────────────────────
    for (const auto& e : j["edges"]) {
        if (!e.is_object()) { std::cerr << "[graph] Skipping non-object edge\n"; continue; }
        try {
            Edge edge;
            edge.from   = e.at("from");
            edge.to     = e.at("to");
            edge.length = e.value("length", 1.0);

            if (graph.node_map.find(edge.from) == graph.node_map.end()) {
                std::cerr << "[graph] Edge from " << edge.from << " references missing node, skipping\n";
                continue;
            }
            if (graph.node_map.find(edge.to) == graph.node_map.end()) {
                std::cerr << "[graph] Edge to " << edge.to << " references missing node, skipping\n";
                continue;
            }

            graph.edges.push_back(edge);
            graph.adj[edge.from].push_back({ edge.to,   edge.length });
            graph.adj[edge.to  ].push_back({ edge.from, edge.length });
        } catch (const json::exception& e) {
            std::cerr << "[graph] Skipping bad edge: " << e.what() << "\n";
        }
    }

    std::cout << "[graph] Loaded "
              << graph.nodes.size() << " nodes, "
              << graph.edges.size() << " edges\n";
    return true;
}