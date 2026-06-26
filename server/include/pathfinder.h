#pragma once
#include "graph.h"
#include <vector>
#include <string>

struct RouteResult {
    bool   found;
    double distance;            // total meters
    std::vector<int> node_ids;  // ordered path node IDs
};

struct OutdoorStep {
    std::string instruction;
    double      distance;
};

// Find nearest graph node to a lat/lon point
int nearestNode(const Graph& graph, double lat, double lon);

// Dijkstra shortest path
// from_id / to_id are node IDs (not osm_id)
RouteResult findRoute(const Graph& graph, int from_id, int to_id);

// Generate turn-by-turn outdoor directions with left/right turns
// toLat/toLon = destination building coordinates for final "on your left/right/ahead"
std::vector<OutdoorStep> generateOutdoorSteps(
    const std::vector<int>& node_ids,
    const Graph& graph,
    const std::string& fromName,
    const std::string& toName,
    double toLat = 0, double toLon = 0);