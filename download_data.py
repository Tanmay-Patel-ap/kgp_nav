"""
download_data.py
Run once to download all campus map data from OSM.

Usage:  python download_data.py
"""

import osmnx as ox
import json, os
import pandas as pd
import sys

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except:
        pass

PLACE_NAME = "IIT Kharagpur"
OUT_NAV    = "server/data/outdoor"
OUT_REN    = "client/data/outdoor"

os.makedirs(OUT_NAV, exist_ok=True)
os.makedirs(OUT_REN,  exist_ok=True)

PASS = "[OK]"
FAIL = "[FAIL]"
WARN = "[WARN]"

# ── HELPERS ───────────────────────────────────────────────────────
def round_coord(v, d=6):
    return round(float(v), d)

def safe_str(v):
    if v is None: return None
    if isinstance(v, float) and pd.isna(v): return None
    s = str(v).strip()
    return s if s and s.lower() != "nan" else None

def geom_to_coords(geom):
    if geom.geom_type == "Polygon":
        return {"type":"Polygon","coordinates":[[[round_coord(x),round_coord(y)] for x,y in geom.exterior.coords]]}
    elif geom.geom_type == "MultiPolygon":
        return {"type":"MultiPolygon","coordinates":[[[[round_coord(x),round_coord(y)] for x,y in p.exterior.coords]] for p in geom.geoms]}
    elif geom.geom_type == "LineString":
        return {"type":"LineString","coordinates":[[round_coord(x),round_coord(y)] for x,y in geom.coords]}
    return None

def save_geojson(path, features):
    try:
        with open(path, "w", encoding="utf-8") as f:
            json.dump({"type":"FeatureCollection","features":features}, f)
    except (OSError, IOError, json.JSONDecodeError) as e:
        print(f"{FAIL} Failed to write {path}: {e}")
    except Exception as e:
        print(f"{FAIL} Unexpected error writing {path}: {e}")

def section(title):
    print("\n" + "-" * 50)
    print(f"  {title}")
    print("-" * 50)

def verify(label, ok, detail=""):
    icon = PASS if ok else FAIL
    print(f"{icon} {label}" + (f"  [{detail}]" if detail else ""))
    return ok

# ═══════════════════════════════════════════════════
# 1. GRAPH
# ═══════════════════════════════════════════════════
section("1/6  WALK GRAPH")

try:
    G_raw  = ox.graph_from_place(PLACE_NAME, network_type="walk", simplify=False, retain_all=True)
    G_proj = ox.project_graph(G_raw)
except Exception as e:
    print(f"{FAIL} OSMnx graph download failed: {e}")
    print("  Check network connectivity and OSM server availability.")
    sys.exit(1)

if not G_raw or len(G_raw.nodes) == 0:
    print(f"{FAIL} Empty graph returned from OSM")
    sys.exit(1)

node_id_map = {osm_id: i for i, osm_id in enumerate(G_raw.nodes())}

nodes_out = []
for osm_id in G_raw.nodes():
    try:
        d  = G_raw.nodes[osm_id]
        dp = G_proj.nodes[osm_id]
        nodes_out.append({
            "id": node_id_map[osm_id], "osm_id": int(osm_id),
            "lat": round_coord(d["y"]),  "lon": round_coord(d["x"]),
            "x":   round(float(dp["x"]),2), "y": round(float(dp["y"]),2),
        })
    except (KeyError, ValueError, TypeError) as e:
        print(f"{WARN} Skipping bad node {osm_id}: {e}")

seen_edges = set()
edges_out  = []
for u, v, ed in G_raw.edges(data=True):
    try:
        u_id = node_id_map.get(u)
        v_id = node_id_map.get(v)
        if u_id is None or v_id is None: continue
        key = (min(u_id, v_id), max(u_id, v_id))
        if key in seen_edges: continue
        seen_edges.add(key)
        edges_out.append({"from": u_id, "to": v_id,
                          "length": round(float(ed.get("length",1.0)),2)})
    except (ValueError, TypeError) as e:
        print(f"{WARN} Skipping bad edge {u}->{v}: {e}")

graph_path = f"{OUT_NAV}/campus_graph.json"
try:
    with open(graph_path, "w") as f:
        json.dump({"nodes": nodes_out, "edges": edges_out}, f)
    print(f"\n  Saved → {graph_path}")
except (OSError, IOError) as e:
    print(f"{FAIL} Cannot write {graph_path}: {e}")
    sys.exit(1)

print(f"\n  VERIFICATION")

# check 1: counts
verify("Node count > 0",   len(nodes_out) > 0,  f"{len(nodes_out)} nodes")
verify("Edge count > 0",   len(edges_out) > 0,  f"{len(edges_out)} edges")

# check 2: unique node IDs
unique_ids = len(set(n["id"] for n in nodes_out))
verify("All node IDs unique", unique_ids == len(nodes_out), f"{unique_ids} unique")

# check 3: no duplicate edges
all_edge_keys = [(min(e["from"],e["to"]), max(e["from"],e["to"])) for e in edges_out]
verify("No duplicate edges", len(all_edge_keys) == len(set(all_edge_keys)))

# check 4: edge references valid nodes
valid_ids  = set(n["id"] for n in nodes_out)
bad_edges  = [e for e in edges_out if e["from"] not in valid_ids or e["to"] not in valid_ids]
verify("All edges reference valid nodes", len(bad_edges) == 0,
       f"{len(bad_edges)} bad" if bad_edges else "")

# check 5: coordinate range (IIT KGP is ~22.31°N 87.31°E)
lat_ok = all(22.28 < n["lat"] < 22.35 for n in nodes_out)
lon_ok = all(87.28 < n["lon"] < 87.35 for n in nodes_out)
verify("Node coordinates in IIT KGP bounds", lat_ok and lon_ok)

# check 6: no zero-length edges
zero_len = [e for e in edges_out if e["length"] <= 0]
verify("No zero-length edges", len(zero_len) == 0,
       f"{len(zero_len)} found" if zero_len else "")

print(f"\n  Sample node: {nodes_out[0]}")
print(f"  Sample edge: {edges_out[0]}")

# ═══════════════════════════════════════════════════
# 2. BUILDINGS JSON (navigation)
# ═══════════════════════════════════════════════════
section("2/6  BUILDINGS JSON  (named, for navigation)")

try:
    buildings_raw = ox.features_from_place(PLACE_NAME, tags={"building": True})
except Exception as e:
    print(f"{FAIL} OSMnx building features download failed: {e}")
    sys.exit(1)

if buildings_raw is None or buildings_raw.empty:
    print(f"{FAIL} No building features returned")
    sys.exit(1)

buildings_out = []
seen_names    = set()
bid           = 0
skipped_noname = 0
skipped_dup    = 0
bad_node_ref   = 0

for _, row in buildings_raw.iterrows():
    name = safe_str(row.get("name"))
    if not name:
        skipped_noname += 1
        continue
    if name in seen_names:
        skipped_dup += 1
        continue
    seen_names.add(name)

    centroid    = row.geometry.centroid
    lat, lon    = round_coord(centroid.y), round_coord(centroid.x)
    nearest_osm = ox.distance.nearest_nodes(G_raw, lon, lat)
    node_id     = node_id_map.get(int(nearest_osm), -1)
    if node_id == -1: bad_node_ref += 1

    buildings_out.append({
        "id": bid, "name": name, "lat": lat, "lon": lon,
        "node_id": node_id, "osm_node_id": int(nearest_osm),
    })
    bid += 1

bld_path = f"{OUT_NAV}/campus_buildings.json"
with open(bld_path, "w") as f:
    json.dump(buildings_out, f, indent=2)

print(f"\n  Saved → {bld_path}")
print(f"\n  VERIFICATION")

verify("Named buildings found",       len(buildings_out) > 0,   f"{len(buildings_out)} buildings")
verify("No duplicate names",          skipped_dup == 0,          f"{skipped_dup} skipped")
verify("All node references valid",   bad_node_ref == 0,         f"{bad_node_ref} bad refs")

# check coords in bounds
b_lat_ok = all(22.28 < b["lat"] < 22.35 for b in buildings_out)
b_lon_ok = all(87.28 < b["lon"] < 87.35 for b in buildings_out)
verify("Building coords in campus bounds", b_lat_ok and b_lon_ok)

# check unique IDs
uniq_bids = len(set(b["id"] for b in buildings_out))
verify("All building IDs unique", uniq_bids == len(buildings_out))

print(f"\n  Skipped (unnamed): {skipped_noname}")
print(f"  Skipped (duplicate name): {skipped_dup}")
print(f"\n  Sample: {buildings_out[0]}")

# ═══════════════════════════════════════════════════
# 3. BUILDING SHAPES (rendering)
# ═══════════════════════════════════════════════════
section("3/6  BUILDING SHAPES  (all polygons, for rendering)")

shape_features = []
skipped_geom   = 0
sfid = 0

for _, row in buildings_raw.iterrows():
    geom = row.geometry
    if geom.geom_type not in ("Polygon","MultiPolygon"):
        skipped_geom += 1
        continue
    geo = geom_to_coords(geom)
    if not geo:
        skipped_geom += 1
        continue
    shape_features.append({
        "type": "Feature",
        "properties": {"id": sfid, "name": safe_str(row.get("name"))},
        "geometry": geo,
    })
    sfid += 1

shapes_path = f"{OUT_REN}/building_shapes.geojson"
save_geojson(shapes_path, shape_features)

print(f"\n  Saved → {shapes_path}")
print(f"\n  VERIFICATION")

verify("Shape count > 0",          len(shape_features) > 0,    f"{len(shape_features)} shapes")
verify("Shapes >= buildings_json", len(shape_features) >= len(buildings_out),
       f"{len(shape_features)} shapes vs {len(buildings_out)} named")

# check all polygons have >= 3 points
bad_poly = 0
for f in shape_features:
    geo = f["geometry"]
    if geo["type"] == "Polygon":
        if len(geo["coordinates"][0]) < 3: bad_poly += 1
    elif geo["type"] == "MultiPolygon":
        for ring in geo["coordinates"]:
            if len(ring[0]) < 3: bad_poly += 1
verify("All polygons have ≥ 3 points", bad_poly == 0, f"{bad_poly} bad" if bad_poly else "")

# named vs unnamed ratio
named_shapes   = sum(1 for f in shape_features if f["properties"]["name"])
unnamed_shapes = len(shape_features) - named_shapes
verify("Has both named and unnamed shapes",
       named_shapes > 0 and unnamed_shapes >= 0,
       f"{named_shapes} named, {unnamed_shapes} unnamed")

print(f"\n  Skipped (non-polygon geometry): {skipped_geom}")

# ═══════════════════════════════════════════════════
# 4. ROADS (rendering)
# ═══════════════════════════════════════════════════
section("4/6  ROADS  (deduplicated)")

road_features  = []
seen_road_keys = set()
rid            = 0
hw_types       = {}

for u, v, ed in G_raw.edges(data=True):
    key = (min(u,v), max(u,v))
    if key in seen_road_keys: continue
    seen_road_keys.add(key)

    if "geometry" in ed:
        coords = [[round_coord(x), round_coord(y)] for x, y in ed["geometry"].coords]
    else:
        fn, tn = G_raw.nodes[u], G_raw.nodes[v]
        coords = [[round_coord(fn["x"]),round_coord(fn["y"])],
                  [round_coord(tn["x"]),round_coord(tn["y"])]]

    hw = safe_str(ed.get("highway"))
    if isinstance(hw, list): hw = hw[0]
    hw_types[hw] = hw_types.get(hw, 0) + 1

    road_features.append({
        "type": "Feature",
        "properties": {
            "id": rid, "name": safe_str(ed.get("name")),
            "highway": hw, "length": round(float(ed.get("length",0)),2),
        },
        "geometry": {"type":"LineString","coordinates":coords},
    })
    rid += 1

roads_path = f"{OUT_REN}/roads.geojson"
save_geojson(roads_path, road_features)

print(f"\n  Saved → {roads_path}")
print(f"\n  VERIFICATION")

verify("Road count > 0", len(road_features) > 0, f"{len(road_features)} roads")

# check deduplication worked
all_rkeys = [(f["properties"]["id"]) for f in road_features]
verify("All road IDs unique", len(all_rkeys) == len(set(all_rkeys)))

# check no roads with < 2 points
bad_roads = [f for f in road_features if len(f["geometry"]["coordinates"]) < 2]
verify("All roads have ≥ 2 coords", len(bad_roads) == 0,
       f"{len(bad_roads)} bad" if bad_roads else "")

# check no zero-length roads
zero_roads = [f for f in road_features if f["properties"]["length"] <= 0]
verify("No zero-length roads", len(zero_roads) == 0,
       f"{len(zero_roads)} found" if zero_roads else "")

print(f"\n  Highway type breakdown:")
for hw, count in sorted(hw_types.items(), key=lambda x: -x[1]):
    print(f"    {str(hw):<20} {count}")

# ═══════════════════════════════════════════════════
# 5. CAMPUS BOUNDARY
# ═══════════════════════════════════════════════════
section("5/6  CAMPUS BOUNDARY")

try:
    boundary_gdf   = ox.geocode_to_gdf(PLACE_NAME)
except Exception as e:
    print(f"{FAIL} OSMnx geocode failed: {e}")
    sys.exit(1)

if boundary_gdf is None or boundary_gdf.empty:
    print(f"{FAIL} No boundary data returned")
    sys.exit(1)

boundary_feats = []

for _, row in boundary_gdf.iterrows():
    geo = geom_to_coords(row.geometry)
    if geo:
        boundary_feats.append({
            "type": "Feature",
            "properties": {"name": PLACE_NAME},
            "geometry": geo,
        })

boundary_path = f"{OUT_REN}/campus_boundary.geojson"
save_geojson(boundary_path, boundary_feats)

print(f"\n  Saved → {boundary_path}")
print(f"\n  VERIFICATION")

verify("Boundary found",     len(boundary_feats) > 0, f"{len(boundary_feats)} polygon(s)")
verify("Single boundary",    len(boundary_feats) == 1,
       "multiple found — check OSM" if len(boundary_feats) > 1 else "")

if boundary_feats:
    geo = boundary_feats[0]["geometry"]
    n_pts = len(geo["coordinates"][0]) if geo["type"] == "Polygon" else \
            sum(len(r[0]) for r in geo["coordinates"])
    verify("Boundary has sufficient points", n_pts >= 10, f"{n_pts} points")

# ═══════════════════════════════════════════════════
# 6. EXTRA FEATURES (water, parks, sports)
# ═══════════════════════════════════════════════════
section("6/6  EXTRA FEATURES  (water, parks, sports, parking)")

extra_tags = {
    "natural": ["water","wood","tree_row"],
    "leisure": ["park","garden","pitch","sports_centre","swimming_pool"],
    "landuse": ["grass","forest","recreation_ground"],
    "amenity": ["parking"],
}

extra_features = []
extra_counts   = {}
eid = 0

for tag_key, tag_values in extra_tags.items():
    for tag_val in tag_values:
        try:
            feats = ox.features_from_place(PLACE_NAME, tags={tag_key: tag_val})
            count = 0
            for _, row in feats.iterrows():
                geom = row.geometry
                if geom.geom_type not in ("Polygon","MultiPolygon","LineString"): continue
                geo = geom_to_coords(geom)
                if not geo: continue
                extra_features.append({
                    "type": "Feature",
                    "properties": {
                        "id": eid, "name": safe_str(row.get("name")),
                        "category": tag_key, "type": tag_val,
                    },
                    "geometry": geo,
                })
                eid += 1
                count += 1
            if count > 0:
                extra_counts[f"{tag_key}={tag_val}"] = count
        except Exception:
            pass

features_path = f"{OUT_REN}/features.geojson"
save_geojson(features_path, extra_features)

print(f"\n  Saved → {features_path}")
print(f"\n  VERIFICATION")

verify("Extra features found", len(extra_features) > 0, f"{len(extra_features)} total")

bad_extra = [f for f in extra_features if f["properties"]["category"] is None]
verify("All features have category", len(bad_extra) == 0)

print(f"\n  Feature type breakdown:")
for tag, count in sorted(extra_counts.items(), key=lambda x: -x[1]):
    print(f"    {tag:<30} {count}")

# ═══════════════════════════════════════════════════
# FINAL SUMMARY
# ═══════════════════════════════════════════════════
print("\n" + "=" * 50)
print("  DOWNLOAD COMPLETE - SUMMARY")
print("=" * 50)

print(f"  Nodes:          {len(nodes_out)}")
print(f"  Edges:          {len(edges_out)}")
print(f"  Buildings:      {len(buildings_out)} named")
print(f"  Shapes:         {len(shape_features)} polygons")
print(f"  Roads:          {len(road_features)} segments")
print(f"  Extra features: {len(extra_features)}")

print("=" * 50 + "\n")