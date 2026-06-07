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

    for (const auto& b : j) {
        // Skip unnamed entries
        if (!b.contains("name") || b["name"].is_null()) continue;
        std::string name = b["name"];
        if (name.empty() || name == "nan") continue;

        Building bld;
        bld.id          = b["id"];
        bld.name        = name;
        bld.lat         = b["lat"];
        bld.lon         = b["lon"];
        bld.node_id     = b["node_id"];
        bld.osm_node_id = b["osm_node_id"];

        buildings.push_back(bld);
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