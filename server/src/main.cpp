#include "graph.h"
#include "buildings.h"
#include "server.h"
#include "indoor.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <unordered_map>

int main(int argc, char* argv[]) {

    // ── DEFAULTS ───────────────────────────────────────────────
    std::string dataDir = "server/data";
    int         port    = 8080;

    if (argc >= 2) {
        char* end = nullptr;
        long p = std::strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || p <= 0 || p > 65535) {
            std::cerr << "[main] Invalid port: " << argv[1] << ". Using 8080.\n";
        } else {
            port = static_cast<int>(p);
        }
    }
    if (argc >= 3) {
        dataDir = argv[2];
    }

    std::string graphPath     = dataDir + "/outdoor/campus_graph.json";
    std::string buildingsPath = dataDir + "/outdoor/campus_buildings.json";

    // ── CHECK DATA FILES EXIST ─────────────────────────────────
    auto fileExists = [](const std::string& p) {
        std::ifstream f(p);
        return f.is_open();
    };
    if (!fileExists(graphPath)) {
        std::cerr << "[main] Graph file not found: " << graphPath << "\n";
        std::cerr << "[main] Run from project root or pass data dir as second arg.\n";
    }
    if (!fileExists(buildingsPath)) {
        std::cerr << "[main] Buildings file not found: " << buildingsPath << "\n";
    }

    // ── LOAD GRAPH ─────────────────────────────────────────────
    Graph graph;
    if (!loadGraph(graphPath, graph)) {
        std::cerr << "[main] Failed to load graph. Exiting.\n";
        return 1;
    }

    // ── LOAD BUILDINGS ─────────────────────────────────────────
    std::vector<Building> buildings;
    if (!loadBuildings(buildingsPath, buildings)) {
        std::cerr << "[main] Failed to load buildings. Exiting.\n";
        return 1;
    }

    // ── LOAD INDOOR GRAPHS ────────────────────────────────────
    std::unordered_map<std::string, IndoorGraph> indoorGraphs;
    std::string indoorDir = dataDir + "/indoor";
    if (loadAllIndoorGraphs(indoorDir, indoorGraphs)) {
        std::cout << "[main] Indoor: " << indoorGraphs.size() << " building(s) loaded\n";
    } else {
        std::cerr << "[main] No indoor data loaded — indoor navigation disabled\n";
    }

    // ── START SERVER ───────────────────────────────────────────
    ServerConfig config;
    config.host = "0.0.0.0";
    config.port = port;

    runServer(config, graph, buildings, indoorGraphs);

    return 0;
}