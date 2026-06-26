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
    const ServerConfig&                                       config,
    const Graph&                                              graph,
    const std::vector<Building>&                              buildings,
    const std::unordered_map<std::string, IndoorGraph>&        indoorGraphs)
{
    httplib::Server svr;

    // ── PREFLIGHT (CORS OPTIONS) ───────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        setCORS(res);
        res.status = 204;
    });

    // ── GET /buildings ─────────────────────────────────────────
    // Returns full building list for client autocomplete preload
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

        if (from_node == -1) { sendError(res, "Origin has no reachable graph node", 404); return; }
        if (to_node == -1)   { sendError(res, "Destination has no reachable graph node", 404); return; }

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

        // Generate turn-by-turn steps
        auto outSteps = generateOutdoorSteps(route.node_ids, graph,
            from_bld->name, to_bld->name, to_bld->lat, to_bld->lon);
        json stepsArr = json::array();
        for (const auto& s : outSteps) {
            stepsArr.push_back({{"text", s.instruction}, {"distance", s.distance}});
        }

        sendJson(res, {
            { "path",     path           },
            { "distance", route.distance },
            { "steps",    stepsArr       },
        });
    });

    // ── GET /outdoor/graph ────────────────────────────────────────
    // Returns full campus graph with nodes + edges
    svr.Get("/outdoor/graph", [&](const httplib::Request&, httplib::Response& res) {
        if (graph.nodes.empty()) {
            std::cerr << "[server] GET /outdoor/graph — graph not loaded\n";
            sendError(res, "Graph not loaded", 503);
            return;
        }
        std::cout << "[server] GET /outdoor/graph — " << graph.nodes.size() << " nodes, "
                  << graph.edges.size() << " edges\n";
        json nodesObj = json::object();
        for (const auto& n : graph.nodes) {
            nodesObj[std::to_string(n.id)] = {
                {"id", n.id}, {"osm_id", n.osm_id},
                {"lat", n.lat}, {"lon", n.lon},
                {"x", n.x}, {"y", n.y}
            };
        }
        json edgesArr = json::array();
        for (const auto& e : graph.edges) {
            edgesArr.push_back({{"from", e.from}, {"to", e.to}, {"length", e.length}});
        }
        sendJson(res, {{"nodes", nodesObj}, {"edges", edgesArr}});
    });

    // ── GET /indoor ─────────────────────────────────────────────
    // Returns list of available indoor buildings with metadata
    svr.Get("/indoor", [&](const httplib::Request&, httplib::Response& res) {
        std::cout << "[server] GET /indoor → " << indoorGraphs.size() << " buildings\n";
        json arr = json::array();
        for (const auto& [prefix, ig] : indoorGraphs) {
            json blocksArr = json::array();
            for (const auto& b : ig.blocks) {
                blocksArr.push_back({ {"name", b.name}, {"floors", b.floors} });
            }
            arr.push_back({
                {"id", ig.id},
                {"prefix", ig.prefix},
                {"name", ig.name},
                {"total_rooms", ig.totalRooms},
                {"blocks", blocksArr},
            });
        }
        sendJson(res, arr);
    });

    // ── GET /indoor/graph?b=PREFIX ──────────────────────────────
    // Returns full graph JSON for a building (for room dropdowns)
    svr.Get("/indoor/graph", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("b")) { sendError(res, "Missing parameter: b"); return; }
        std::string prefix = req.get_param_value("b");
        auto it = indoorGraphs.find(prefix);
        if (it == indoorGraphs.end()) {
            std::cerr << "[server] GET /indoor/graph?b=" << prefix << " — not found\n";
            sendError(res, "Building not found: " + prefix, 404);
            return;
        }
        const IndoorGraph& ig = it->second;
        std::cout << "[server] GET /indoor/graph?b=" << prefix << " — " << ig.nodes.size() << " nodes\n";
        json nodesArr = json::array();
        for (const auto& n : ig.nodes) {
            nodesArr.push_back({{"id", n.id},{"name", n.name},{"type", n.type},{"block", n.block},{"blockIdx", n.blockIdx},{"floor", n.floor}});
        }
        json blocksArr = json::array();
        for (const auto& b : ig.blocks) blocksArr.push_back({{"name", b.name},{"floors", b.floors}});
        sendJson(res, {{"prefix", ig.prefix},{"name", ig.name},{"blocks", blocksArr},{"nodes", nodesArr}});
    });

    // ── GET /indoor/navigate?b=PREFIX&from=NODE_ID&to=NODE_ID ───
    // Runs Dijkstra on indoor graph, returns path + textual steps
    svr.Get("/indoor/navigate", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("b") || !req.has_param("from") || !req.has_param("to")) {
            sendError(res, "Missing parameters: b, from, to");
            return;
        }

        std::string prefix = req.get_param_value("b");
        std::string fromId = req.get_param_value("from");
        std::string toId   = req.get_param_value("to");

        auto it = indoorGraphs.find(prefix);
        if (it == indoorGraphs.end()) {
            sendError(res, "Building not found: " + prefix, 404);
            return;
        }

        const IndoorGraph& ig = it->second;
        std::cout << "[server] GET /indoor/navigate?b=" << prefix << "&from=" << fromId << "&to=" << toId << "\n";
        IndoorRouteResult route = navigateIndoors(ig, fromId, toId);

        if (!route.found) {
            std::cerr << "[server] No route found for " << prefix << ": " << fromId << " → " << toId << "\n";
            sendError(res, "No route found", 404);
            return;
        }

        json stepsArr = json::array();
        for (const auto& s : route.steps) {
            stepsArr.push_back({ {"text", s.text}, {"distance", s.distance} });
        }

        json pathArr = json::array();
        for (const auto& nid : route.nodeIds) pathArr.push_back(nid);

        sendJson(res, {
            {"found", true},
            {"distance", route.distance},
            {"path", pathArr},
            {"steps", stepsArr},
        });
    });

    // ── GET /combined-route?from_bld=ID&from_room=NODE_ID&to_bld=ID&to_room=NODE_ID ──
    // Integrated indoor + outdoor routing.
    // from_room / to_room are optional (empty = use building entrance/main gate).
    svr.Get("/combined-route", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("from_bld") || !req.has_param("to_bld")) {
            sendError(res, "Missing parameters: from_bld, to_bld");
            return;
        }

        int from_bld_id, to_bld_id;
        try {
            from_bld_id = std::stoi(req.get_param_value("from_bld"));
            to_bld_id   = std::stoi(req.get_param_value("to_bld"));
        } catch (...) {
            sendError(res, "Parameters from_bld/to_bld must be integers");
            return;
        }

        std::string from_room = req.get_param_value("from_room");
        std::string to_room   = req.get_param_value("to_room");

        // Find buildings by id
        const Building* from_bld = nullptr;
        const Building* to_bld   = nullptr;
        for (const auto& b : buildings) {
            if (b.id == from_bld_id) from_bld = &b;
            if (b.id == to_bld_id)   to_bld   = &b;
        }
        if (!from_bld) { sendError(res, "Origin building not found", 404); return; }
        if (!to_bld)   { sendError(res, "Destination building not found", 404); return; }

        // Helper: find indoor graph by building ID
        auto findIndoorByBldId = [&](int bldId) -> const IndoorGraph* {
            for (const auto& [prefix, ig] : indoorGraphs) {
                if (ig.id == bldId) return &ig;
            }
            return nullptr;
        };

        // Helper: find entrance node ID in an indoor graph
        auto findEntranceNode = [](const IndoorGraph& ig) -> std::string {
            for (const auto& n : ig.nodes) {
                if (n.type == "entrance") return n.id;
            }
            return "";
        };

        // ── Indoor segments ──────────────────────────────────
        json combinedSteps  = json::array();
        double indoorFromDist = 0.0;
        double indoorToDist   = 0.0;
        bool   hasIndoorFrom  = false;
        bool   hasIndoorTo    = false;

        // Origin indoor: from_room → entrance
        if (!from_room.empty()) {
            const IndoorGraph* ig = findIndoorByBldId(from_bld_id);
            if (!ig) {
                std::cerr << "[combined] Origin building " << from_bld_id << " has no indoor graph\n";
            } else {
                std::string entrance = findEntranceNode(*ig);
                if (entrance.empty()) {
                    std::cerr << "[combined] No entrance node in indoor graph for " << ig->prefix << "\n";
                } else if (from_room != entrance) {
                    IndoorRouteResult ir = navigateIndoors(*ig, from_room, entrance);
                    if (ir.found) {
                        hasIndoorFrom = true;
                        indoorFromDist = ir.distance;
                        for (const auto& s : ir.steps) {
                            combinedSteps.push_back({{"text", s.text}, {"distance", s.distance}});
                        }
                        combinedSteps.push_back({{"text", "Exit " + ig->name}, {"distance", 0.0}});
                    } else {
                        std::cerr << "[combined] Indoor route not found in " << ig->prefix << ": " << from_room << " → entrance\n";
                    }
                }
            }
        }

        // Destination indoor: entrance → to_room (saved separately, appended after outdoor)
        json indoorToSteps = json::array();
        if (!to_room.empty()) {
            const IndoorGraph* ig = findIndoorByBldId(to_bld_id);
            if (!ig) {
                std::cerr << "[combined] Destination building " << to_bld_id << " has no indoor graph\n";
            } else {
                std::string entrance = findEntranceNode(*ig);
                if (entrance.empty()) {
                    std::cerr << "[combined] No entrance node in indoor graph for " << ig->prefix << "\n";
                } else if (to_room != entrance) {
                    IndoorRouteResult ir = navigateIndoors(*ig, entrance, to_room);
                    if (ir.found) {
                        hasIndoorTo = true;
                        indoorToDist = ir.distance;
                        indoorToSteps.push_back({{"text", "Enter " + ig->name}, {"distance", 0.0}});
                        for (const auto& s : ir.steps) {
                            indoorToSteps.push_back({{"text", s.text}, {"distance", s.distance}});
                        }
                    } else {
                        std::cerr << "[combined] Indoor route not found in " << ig->prefix << ": entrance → " << to_room << "\n";
                    }
                }
            }
        }

        // ── Outdoor segment ──────────────────────────────────
        int from_node = nearestNode(graph, from_bld->lat, from_bld->lon);
        int to_node   = nearestNode(graph, to_bld->lat,   to_bld->lon);

        if (from_node == -1) { sendError(res, "Origin has no reachable graph node", 404); return; }
        if (to_node == -1)   { sendError(res, "Destination has no reachable graph node", 404); return; }

        RouteResult route = findRoute(graph, from_node, to_node);

        if (!route.found) {
            sendError(res, "No outdoor route found", 404);
            return;
        }

        // Outdoor path as lat/lon array
        json outPath = json::array();
        for (int nid : route.node_ids) {
            auto it = graph.node_map.find(nid);
            if (it == graph.node_map.end()) continue;
            outPath.push_back({{"lat", it->second.lat}, {"lon", it->second.lon}});
        }

        // Turn-by-turn outdoor steps
        auto outSteps = generateOutdoorSteps(route.node_ids, graph,
            from_bld->name, to_bld->name, to_bld->lat, to_bld->lon);
        for (const auto& s : outSteps) {
            combinedSteps.push_back({{"text", s.instruction}, {"distance", s.distance}});
        }

        // Append destination indoor steps after outdoor
        for (const auto& s : indoorToSteps) {
            combinedSteps.push_back(s);
        }

        // ── Combined response ────────────────────────────────
        double totalDistance = route.distance + indoorFromDist + indoorToDist;

        sendJson(res, {
            {"distance", totalDistance},
            {"outdoor_distance", route.distance},
            {"indoor_from_distance", indoorFromDist},
            {"indoor_to_distance", indoorToDist},
            {"path", outPath},
            {"steps", combinedSteps},
            {"has_indoor_from", hasIndoorFrom},
            {"has_indoor_to", hasIndoorTo},
        });
    });

    // ── START ──────────────────────────────────────────────────
    std::cout << "[server] Listening on "
              << config.host << ":" << config.port << "\n";

    if (!svr.listen(config.host, config.port)) {
        std::cerr << "[server] Failed to bind port " << config.port << "\n";
    }
}