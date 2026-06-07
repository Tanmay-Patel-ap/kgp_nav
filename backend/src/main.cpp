#include "graph.h"
#include "buildings.h"
#include "server.h"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {

    // ── DATA PATHS ─────────────────────────────────────────────
    // Relative to where the binary is run from (project root)
    std::string graphPath     = "data/outdoor/navigation/campus_graph.json";
    std::string buildingsPath = "data/outdoor/navigation/campus_buildings.json";

    // Optional: override via args
    // ./campus_nav [port] [data_dir]
    int port = 8080;
    if (argc >= 2) port = std::atoi(argv[1]);
    if (argc >= 3) {
        std::string dataDir = argv[2];
        graphPath     = dataDir + "/outdoor/navigation/campus_graph.json";
        buildingsPath = dataDir + "/outdoor/navigation/campus_buildings.json";
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

    // ── START SERVER ───────────────────────────────────────────
    ServerConfig config;
    config.host = "0.0.0.0";
    config.port = port;

    runServer(config, graph, buildings);

    return 0;
}