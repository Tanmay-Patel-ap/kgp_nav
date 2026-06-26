#pragma once
#include <vector>
#include <string>

struct Building {
    int         id;
    std::string name;
    double      lat;
    double      lon;
    int         node_id;
    long        osm_node_id;
};

// Load from campus_buildings.json
bool loadBuildings(const std::string& path, std::vector<Building>& buildings);

// Case-insensitive substring search, returns up to `limit` results
std::vector<Building> searchBuildings(
    const std::vector<Building>& buildings,
    const std::string& query,
    int limit = 8
);