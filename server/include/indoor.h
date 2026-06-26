#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct IndoorNode {
    std::string id;
    std::string name;
    std::string type;
    std::string block;
    int         blockIdx;
    int         floor;
};

struct IndoorEdge {
    std::string from;
    std::string to;
    double      weight;
    std::string type; // "corridor", "staircase", "elevator", etc.
};

struct IndoorConnection {
    std::string type;
    int         fromLevel;
    std::string fromNode;
    int         toLevel;
    std::string toNode;
    double      weight;
};

struct IndoorBlockInfo {
    std::string name;
    int         floors;
};

struct IndoorGraph {
    int                                id;          // matches outdoor building id
    std::string                        prefix;
    std::string                        name;
    std::vector<IndoorBlockInfo>       blocks;
    int                                totalRooms;
    std::vector<IndoorNode>            nodes;
    std::unordered_map<std::string,int>          nodeIndex;   // id → index in nodes[]
    std::vector<IndoorEdge>            edges;
    std::vector<IndoorConnection>      connections;
    // adjacency built at load time
    std::unordered_map<std::string, std::vector<std::pair<std::string, double>>> adj;
};

struct IndoorNavStep {
    std::string text;
    double      distance;
};

struct IndoorRouteResult {
    bool                found;
    double              distance;
    std::vector<std::string> nodeIds;
    std::vector<IndoorNavStep>   steps;
};

// Scan data/indoor/*.json and load all graphs
// maps prefix → IndoorGraph
bool loadAllIndoorGraphs(const std::string& directory,
                         std::unordered_map<std::string, IndoorGraph>& out);

// Dijkstra + direction generation
IndoorRouteResult navigateIndoors(const IndoorGraph& graph,
                                  const std::string& fromId,
                                  const std::string& toId);
