import osmnx as ox
import json
import pandas as pd


PLACE_NAME = "IIT Kharagpur"


def export_buildings():

    print(f"Downloading buildings for {PLACE_NAME}...")

    # --------------------------------
    # LOAD OUTDOOR GRAPH
    # --------------------------------
    G = ox.graph_from_place(
        PLACE_NAME,
        network_type="walk",
        simplify=False,
        retain_all=True
    )

    # --------------------------------
    # LOAD INTERNAL NODE ID MAPPING
    # --------------------------------
    with open("data/outdoor/navigation/campus_graph.json", "r") as f:
        graph_data = json.load(f)

    osm_to_internal = {}

    for node in graph_data["nodes"]:
        osm_to_internal[node["osm_id"]] = node["id"]

    # --------------------------------
    # DOWNLOAD BUILDINGS
    # --------------------------------
    buildings = ox.features_from_place(
        PLACE_NAME,
        tags={"building": True}
    )

    building_data = []

    building_id = 0

    # --------------------------------
    # PROCESS BUILDINGS
    # --------------------------------
    for _, row in buildings.iterrows():

        # skip unnamed buildings
        if "name" not in row:
            continue
        
        if pd.isna(row["name"]):
            continue
        
        name = str(row["name"]).strip()

        if not name:
            continue
        
        if name.lower() == "nan":
            continue

        # geometry center
        centroid = row.geometry.centroid

        lon = round(centroid.x, 6)
        lat = round(centroid.y, 6)

        # nearest OSM node
        nearest_node = ox.distance.nearest_nodes(
            G,
            lon,
            lat
        )

        # convert OSM node ID -> internal node ID
        osm_id = int(nearest_node)
        internal_node_id = osm_to_internal[osm_id]

        building = {
            "id": building_id,

            "name": name,

            "lat": lat,
            "lon": lon,

            "node_id":  internal_node_id,
            "osm_node_id": osm_id       }

        building_data.append(building)

        building_id += 1

    # --------------------------------
    # SAVE JSON
    # --------------------------------
    output_path = "data/outdoor/navigation/campus_buildings.json"

    with open(output_path, "w") as f:
        json.dump(building_data, f, indent=4)

    print("campus_buildings.json created successfully")

    # --------------------------------
    # VALIDATION
    # --------------------------------
    print()
    print("BUILDINGS VALIDATION")
    print("-" * 30)

    print("Total Buildings:", len(building_data))

    # -----------------------------
    # CHECK UNIQUE BUILDING IDS
    # -----------------------------
    building_ids = set()

    for building in building_data:
        building_ids.add(building["id"])

    print("Unique Building IDs:", len(building_ids))

    # -----------------------------
    # CHECK DUPLICATE NAMES
    # -----------------------------
    building_names = {}

    duplicate_count = 0

    for building in building_data:

        name = building["name"]

        if name in building_names:
            duplicate_count += 1

        building_names[name] = True

    print("Duplicate Building Names:", duplicate_count)

    # -----------------------------
    # CHECK INVALID NODE IDS
    # -----------------------------
    valid_node_ids = set()

    for node in graph_data["nodes"]:
        valid_node_ids.add(node["id"])

    invalid_nodes = 0

    for building in building_data:

        if building["node_id"] not in valid_node_ids:
            invalid_nodes += 1

    print("Invalid Node References:", invalid_nodes)

    # -----------------------------
    # SAMPLE BUILDING
    # -----------------------------
    print()
    print("Sample Building:")
    print(building_data[0])


if __name__ == "__main__":
    export_buildings()