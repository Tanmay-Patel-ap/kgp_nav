#include "server.h"
#include "pathfinder.h"
#include "buildings.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

using json = nlohmann::json;

// ── CORS HEADERS ──────────────────────────────────────────────────
static void setCORS(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ── HELPERS ───────────────────────────────────────────────────────
static json buildingToJson(const Building& b) {
    return {
        { "id",          b.id          },
        { "name",        b.name        },
        { "lat",         b.lat         },
        { "lon",         b.lon         },
        { "node_id",     b.node_id     },
        { "osm_node_id", b.osm_node_id },
    };
}

static void sendJson(httplib::Response& res, const json& j, int status = 200) {
    setCORS(res);
    res.status = status;
    res.set_content(j.dump(), "application/json");
}

static void sendError(httplib::Response& res, const std::string& msg, int status = 400) {
    sendJson(res, { { "error", msg } }, status);
}

// ── SERVER ────────────────────────────────────────────────────────
void runServer(
    const ServerConfig&          config,
    const Graph&                 graph,
    const std::vector<Building>& buildings)
{
    httplib::Server svr;

    // ── PREFLIGHT (CORS OPTIONS) ───────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.status = 204;
    });

    // ── GET /buildings ─────────────────────────────────────────
    // Returns full building list for frontend autocomplete preload
    svr.Get("/buildings", [&](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        for (const auto& b : buildings) arr.push_back(buildingToJson(b));
        sendJson(res, arr);
    });

    // ── GET /search?q=... ──────────────────────────────────────
    // Returns filtered buildings matching query string
    svr.Get("/search", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("q")) {
            sendError(res, "Missing parameter: q");
            return;
        }
        std::string q = req.get_param_value("q");
        auto results  = searchBuildings(buildings, q);

        json arr = json::array();
        for (const auto& b : results) arr.push_back(buildingToJson(b));
        sendJson(res, arr);
    });

    // ── GET /route?from=BUILDING_ID&to=BUILDING_ID ────────────────
    // Accepts building IDs, finds nearest graph node, runs Dijkstra
    svr.Get("/route", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("from") || !req.has_param("to")) {
            sendError(res, "Missing parameters: from, to");
            return;
        }

        int from_bld_id, to_bld_id;
        try {
            from_bld_id = std::stoi(req.get_param_value("from"));
            to_bld_id   = std::stoi(req.get_param_value("to"));
        } catch (...) {
            sendError(res, "Parameters from/to must be integers");
            return;
        }

        // Find buildings by id
        const Building* from_bld = nullptr;
        const Building* to_bld   = nullptr;
        for (const auto& b : buildings) {
            if (b.id == from_bld_id) from_bld = &b;
            if (b.id == to_bld_id)   to_bld   = &b;
        }
        if (!from_bld) { sendError(res, "Origin building not found",      404); return; }
        if (!to_bld)   { sendError(res, "Destination building not found", 404); return; }

        // Find nearest graph nodes to each building
        int from_node = nearestNode(graph, from_bld->lat, from_bld->lon);
        int to_node   = nearestNode(graph, to_bld->lat,   to_bld->lon);

        RouteResult route = findRoute(graph, from_node, to_node);

        if (!route.found) {
            sendError(res, "No route found", 404);
            return;
        }

        // Build path as lat/lon array
        json path = json::array();
        for (int nid : route.node_ids) {
            auto it = graph.node_map.find(nid);
            if (it == graph.node_map.end()) continue;
            path.push_back({
                { "lat", it->second.lat },
                { "lon", it->second.lon },
            });
        }

        sendJson(res, {
            { "path",     path           },
            { "distance", route.distance },
        });
    });

    // ── START ──────────────────────────────────────────────────
    std::cout << "[server] Listening on "
              << config.host << ":" << config.port << "\n";

    if (!svr.listen(config.host, config.port)) {
        std::cerr << "[server] Failed to bind port " << config.port << "\n";
    }
}