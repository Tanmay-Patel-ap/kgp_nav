#include "buildings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

using json = nlohmann::json;

bool loadBuildings(const std::string& path, std::vector<Building>& buildings) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[buildings] Cannot open: " << path << "\n";
        return false;
    }

    json j;
    try {
        f >> j;
    } catch (const json::exception& e) {
        std::cerr << "[buildings] JSON parse error: " << e.what() << "\n";
        return false;
    }

    if (!j.is_array()) {
        std::cerr << "[buildings] Root is not an array\n";
        return false;
    }

    for (const auto& b : j) {
        if (!b.is_object()) { std::cerr << "[buildings] Skipping non-object entry\n"; continue; }

        // Skip unnamed entries
        if (!b.contains("name") || b["name"].is_null()) continue;
        std::string name = b["name"];
        if (name.empty() || name == "nan") continue;

        try {
            Building bld;
            bld.id          = b.at("id");
            bld.name        = name;
            bld.lat         = b.at("lat");
            bld.lon         = b.at("lon");
            bld.node_id     = b.value("node_id", -1);
            bld.osm_node_id = b.value("osm_node_id", -1L);

            // Validate coordinates
            if (bld.lat < -90 || bld.lat > 90) { std::cerr << "[buildings] Invalid lat for " << name << "\n"; continue; }
            if (bld.lon < -180 || bld.lon > 180) { std::cerr << "[buildings] Invalid lon for " << name << "\n"; continue; }

            buildings.push_back(bld);
        } catch (const json::exception& e) {
            std::cerr << "[buildings] Skipping bad entry: " << e.what() << "\n";
        }
    }

    std::cout << "[buildings] Loaded " << buildings.size() << " named buildings\n";
    return true;
}

// ── SEARCH ─────────────────────────────────────────────────────────
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

std::vector<Building> searchBuildings(
    const std::vector<Building>& buildings,
    const std::string& query,
    int limit)
{
    std::string q = toLower(query);
    std::vector<Building> results;

    for (const auto& b : buildings) {
        if (toLower(b.name).find(q) != std::string::npos) {
            results.push_back(b);
            if ((int)results.size() >= limit) break;
        }
    }
    return results;
}