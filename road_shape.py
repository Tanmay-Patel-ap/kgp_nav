import os
import osmnx as ox
import json
import pandas as pd


PLACE_NAME = "IIT Kharagpur"


def export_road_geometry():

    print(f"Downloading road geometry for {PLACE_NAME}...")

    # --------------------------------
    # DOWNLOAD WALKING GRAPH
    # --------------------------------
    G = ox.graph_from_place(
        PLACE_NAME,
        network_type="walk",
        simplify=False,
        retain_all=True
    )

    features = []

    road_id = 0

    # --------------------------------
    # PROCESS EDGES
    # --------------------------------
    for u, v, edge_data in G.edges(data=True):

        coordinates = []

        # --------------------------------
        # USE EXISTING GEOMETRY
        # --------------------------------
        if "geometry" in edge_data:

            geometry = edge_data["geometry"]

            for lon, lat in geometry.coords:

                coordinates.append([
                    round(lon, 6),
                    round(lat, 6)
                ])

        # --------------------------------
        # CREATE GEOMETRY FROM NODES
        # --------------------------------
        else:

            from_node = G.nodes[u]
            to_node = G.nodes[v]

            coordinates = [
                [
                    round(from_node["x"], 6),
                    round(from_node["y"], 6)
                ],
                [
                    round(to_node["x"], 6),
                    round(to_node["y"], 6)
                ]
            ]

        # --------------------------------
        # ROAD NAME
        # --------------------------------
        road_name = None

        if "name" in edge_data:

            if not pd.isna(edge_data["name"]):
                road_name = str(edge_data["name"])

        # --------------------------------
        # ROAD TYPE
        # --------------------------------
        highway_type = None

        if "highway" in edge_data:

            if not pd.isna(edge_data["highway"]):
                highway_type = str(edge_data["highway"])

        # --------------------------------
        # CREATE FEATURE
        # --------------------------------
        feature = {
            "type": "Feature",

            "properties": {
                "id": road_id,

                "name": road_name,

                "highway": highway_type,

                "length": round(
                    float(edge_data.get("length", 0)),
                    2
                )
            },

            "geometry": {
                "type": "LineString",
                "coordinates": coordinates
            }
        }

        features.append(feature)

        road_id += 1

    # --------------------------------
    # FINAL GEOJSON
    # --------------------------------
    geojson_data = {
        "type": "FeatureCollection",
        "features": features
    }

    # --------------------------------

    # --------------------------------
    # SAVE FILE
    # --------------------------------
    output_path = "data/outdoor/rendering/roads.geojson"

    with open(output_path, "w") as f:
        json.dump(geojson_data, f, indent=4)

    print("roads.geojson created successfully")

    # --------------------------------
    # VALIDATION
    # --------------------------------
    print()
    print("ROAD GEOMETRY VALIDATION")
    print("-" * 35)

    print("Total Road Segments:", len(features))

    # --------------------------------
    # CHECK UNIQUE IDS
    # --------------------------------
    ids = set()

    for feature in features:
        ids.add(feature["properties"]["id"])

    print("Unique Road IDs:", len(ids))

    # --------------------------------
    # CHECK INVALID GEOMETRIES
    # --------------------------------
    invalid_geometries = 0

    for feature in features:

        coords = feature["geometry"]["coordinates"]

        if len(coords) < 2:
            invalid_geometries += 1

    print("Invalid Geometries:", invalid_geometries)

    # --------------------------------
    # SAMPLE FEATURE
    # --------------------------------
    print()

    if len(features) > 0:

        print("Sample Feature:")
        print(json.dumps(features[0], indent=4))

    else:
        print("No features found")


if __name__ == "__main__":
    export_road_geometry()