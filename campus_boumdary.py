import os
import osmnx as ox
import json


PLACE_NAME = "IIT Kharagpur"


def export_campus_boundary():

    print(f"Downloading campus boundary for {PLACE_NAME}...")

    # --------------------------------
    # DOWNLOAD BOUNDARY
    # --------------------------------
    boundary = ox.geocode_to_gdf(
        PLACE_NAME
    )

    features = []

    # --------------------------------
    # PROCESS GEOMETRY
    # --------------------------------
    for _, row in boundary.iterrows():

        geometry = row.geometry

        # only polygons
        if geometry.geom_type != "Polygon":
            continue

        coordinates = []

        for lon, lat in geometry.exterior.coords:

            coordinates.append([
                round(lon, 6),
                round(lat, 6)
            ])

        feature = {
            "type": "Feature",

            "properties": {
                "name": PLACE_NAME
            },

            "geometry": {
                "type": "Polygon",
                "coordinates": [coordinates]
            }
        }

        features.append(feature)

    # --------------------------------
    # FINAL GEOJSON
    # --------------------------------
    geojson_data = {
        "type": "FeatureCollection",
        "features": features
    }


    # --------------------------------
    # SAVE FILE
    # --------------------------------
    output_path = "data/outdoor/rendering/campus_boundary.geojson"

    with open(output_path, "w") as f:
        json.dump(geojson_data, f, indent=4)

    print("campus_boundary.geojson created successfully")

    # --------------------------------
    # VALIDATION
    # --------------------------------
    print()
    print("BOUNDARY VALIDATION")
    print("-" * 30)

    print("Total Features:", len(features))

    if len(features) > 0:

        print()
        print("Sample Feature:")
        print(json.dumps(features[0], indent=4))

    else:
        print("No boundary found")


if __name__ == "__main__":
    export_campus_boundary()