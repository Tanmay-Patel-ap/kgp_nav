# 🗺️ KGP Navigator

KGP Navigator is a unified indoor-outdoor navigation system for IIT Kharagpur that combines Dijkstra shortest-path routing across the full campus road network with per-building indoor room-to-room directions. Users can search any building, select a specific room, and get turn-by-turn guidance from origin to destination — all within a single Leaflet-powered map interface.

---

## 🌐 Live Demo

Serve the `client/` directory with any HTTP server (e.g. VS Code Live Server) and run the backend:

**Backend:** `http://127.0.0.1:8080` (self-hosted C++ server)

---

## ✨ Key Features

### Campus Map Visualization

Interactive map of IIT Kharagpur with dark/light themes, building shapes, road networks, water bodies, and campus boundary — all rendered from GeoJSON data via Leaflet.

---

### Building Search & Autocomplete

Real-time search across 60+ named campus buildings with autocomplete dropdown, map fly-to on selection, and persistent state for origin/destination.

---

### 🏠 Room-Level Indoor Navigation

Select a room inside your origin or destination building (e.g. "A 107", "B 401") and get step-by-step indoor directions with corridor turns, staircase/elevator transitions, distance, and arrival confirmation.

---

### Outdoor Turn-by-Turn Directions

Dijkstra shortest path on the 7000+ node campus walk graph generates human-readable directions: "Walk 19m", "Take the right road", "Turn left", with final arrival phrase like "JCB Hall of Residence is on your left".

---

### 🔗 Combined Indoor + Outdoor Routing

Seamlessly navigate from a room in one building to a room in another — the backend stitches together indoor-from + outdoor + indoor-to segments into a single unified itinerary with section headers.

---

## Technology Stack

### Frontend

* HTML5, CSS3, Vanilla JavaScript (ES6+)
* Leaflet 1.9.4 — map rendering
* CartoDB tiles (dark_all / light_all)
* GeoJSON — building shapes, roads, features

### Backend

* C++17 — server and routing algorithms
* cpp-httplib — single-header HTTP library
* nlohmann/json — single-header JSON library
* CMake 3.16+ — build system

### Algorithms & Data

* Dijkstra shortest path (outdoor + indoor)
* OSMnx — OpenStreetMap data acquisition
* Custom JSON graph format (indoor / outdoor)

### Deployment

* Self-hosted C++ binary on any platform
* Any HTTP server for frontend (Live Server, nginx, etc.)

---

## 📁 Project Structure

```text
kgp_nav/
│
├── client/                  # Frontend web application
│   ├── index.html           # Main map & navigation UI
│   ├── css/                 # Stylesheets (map, search, sidebar)
│   ├── js/                  # JavaScript modules (api, layers, map,
│   │                        #   route, search, ui)
│   └── data/                # GeoJSON rendering data
│
├── server/                  # C++ backend
│   ├── include/             # Headers
│   ├── src/                 # Source files (main, server, graph,
│   │                        #   buildings, pathfinder, indoor)
│   ├── data/                # Navigation data
│   │   ├── indoor/          # Per-building indoor graphs
│   │   └── outdoor/         # Campus graph + buildings JSON
│   ├── Cmakelists.txt       # Build configuration
│   └── start.sh             # Build & run script (MSYS2/MinGW)
│
├── screenshots/             # Screenshots
└── README.md
```

---

## 📸 Screenshots

*Screenshots to be added*

### Main Map View

*Map of IIT Kharagpur with building shapes, road network, and feature layers.*

![Main Map View](screenshots/home_page.png)

---

### Building Search

*Autocomplete dropdown showing building search results.*

![Building Search](screenshots/input_sugession.png)

---

### Outdoor Route

*Route polyline drawn on map with turn-by-turn directions in the sidebar.*

![Outdoor Route](screenshots/outdoor.png)

---

### Indoor Navigation

*Room-level directions inside a building with corridor turns, stairs, and elevator transitions.*

![Indoor Navigation](screenshots/indoor.png)

---

### Combined Indoor + Outdoor

*Sectioned route panel showing "inside building → in campus → inside building" with room-level steps.*

![Combined Indoor + Outdoor](screenshots/combined.png)

---

## Usage

### Build & Run Backend

```bash
cd server
./start.sh [port]          # build & run in one step
```

**Manual:**
```bash
cd server
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run from project root:
cd ..
server/build/kgp_nav.exe [port] [data_dir]
# port:     8080 (default)
# data_dir: server/data (default)
```

### Serve Frontend

Open `client/index.html` via any HTTP server (or VS Code Live Server).

### API Endpoints

| Endpoint | Parameters | Description |
|---|---|---|
| `GET /buildings` | — | Full building list |
| `GET /search` | `?q=<query>` | Building name search |
| `GET /route` | `?from=<id>&to=<id>` | Outdoor route with turn-by-turn steps |
| `GET /combined-route` | `?from_bld=<id>&from_room=<id>&to_bld=<id>&to_room=<id>` | Combined indoor+outdoor+indoor route |
| `GET /indoor` | — | List buildings with indoor data |
| `GET /indoor/graph` | `?b=<prefix>` | Full indoor graph for a building |
| `GET /indoor/navigate` | `?b=<prefix>&from=<node>&to=<node>` | Indoor route between two nodes |

---

## 🔮 Future Enhancements

* GPS-based real-time positioning and live tracking
* Additional building interior data (academic blocks, departments, more hall of residences)
* User authentication with saved routes and history
* Voice-guided turn-by-turn navigation
* Accessibility-aware routing (elevators, ramps)
* Public transport integration (auto-rickshaw, bus stops)

---

## Author

**Aditya**

*Built with ❤️ for KGPians!!*
