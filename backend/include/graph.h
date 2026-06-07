#pragma once
#include <vector>
#include <unordered_map>
#include <string>

struct Node {
    int    id;
    long   osm_id;
    double lat;
    double lon;
    double x;
    double y;
};

struct Edge {
    int    from;
    int    to;
    double length;
};

struct Graph {
    std::vector<Node>                             nodes;
    std::vector<Edge>                             edges;
    std::unordered_map<int, Node>                 node_map;   // id → Node
    std::unordered_map<int, std::vector<std::pair<int,double>>> adj; // id → [(neighbor, dist)]
};

// Load from campus_graph.json
// Returns false on failure
bool loadGraph(const std::string& path, Graph& graph);