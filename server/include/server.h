#pragma once
#include "graph.h"
#include "buildings.h"
#include "indoor.h"
#include <string>
#include <unordered_map>

struct ServerConfig {
    std::string host = "0.0.0.0";
    int         port = 8080;
};

// Blocking — runs until process killed
void runServer(
    const ServerConfig&                                       config,
    const Graph&                                              graph,
    const std::vector<Building>&                              buildings,
    const std::unordered_map<std::string, IndoorGraph>&        indoorGraphs
);