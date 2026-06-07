#pragma once
#include "graph.h"
#include "buildings.h"
#include <string>

struct ServerConfig {
    std::string host = "0.0.0.0";
    int         port = 8080;
};

// Blocking — runs until process killed
void runServer(
    const ServerConfig&          config,
    const Graph&                 graph,
    const std::vector<Building>& buildings
);