import osmnx as ox
import json


PLACE_NAME = "IIT Kharagpur"


def export_building_shapes():

    print(f"Downloading building polygons for {PLACE_NAME}...")

    # --------------------------------
    # DOWNLOAD BUILDINGS
    # --------------------------------
    buildings = ox.features_from_place(
        PLACE_NAME,
        tags={"building": True}
    )

    features = []

    building_id = 0

    # --------------------------------
    # PROCESS BUILDINGS
    # --------------------------------
    for _, row in buildings.iterrows():

        # skip unnamed buildings
        if "name" not in row:
            continue

        if not row["name"]:
            continue

        geometry = row.geometry

        # only polygons
        if geometry.geom_type != "Polygon":
            continue

        coordinates = []

        # exterior polygon points
        for lon, lat in geometry.exterior.coords:

            coordinates.append([
                round(lon, 6),
                round(lat, 6)
            ])

        feature = {
            "type": "Feature",

            "properties": {
                "id": building_id,
                "name": str(row["name"]).strip()
            },

            "geometry": {
                "type": "Polygon",
                "coordinates": [coordinates]
            }
        }

        features.append(feature)

        building_id += 1

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
    output_path = "data/outdoor/rendering/building_shapes.geojson"

    with open(output_path, "w") as f:
        json.dump(geojson_data, f, indent=4)

    print("building_shapes.geojson created successfully")

    print()
    print("Total Building Polygons:", len(features))

    print()
    print("Sample Feature:")
    print(features[0])


if __name__ == "__main__":
    export_building_shapes()