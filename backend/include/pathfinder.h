#pragma once
#include "graph.h"
#include <vector>

struct RouteResult {
    bool   found;
    double distance;            // total meters
    std::vector<int> node_ids;  // ordered path node IDs
};

// Find nearest graph node to a lat/lon point
int nearestNode(const Graph& graph, double lat, double lon);

// Dijkstra shortest path
// from_id / to_id are node IDs (not osm_id)
RouteResult findRoute(const Graph& graph, int from_id, int to_id);