import osmnx as ox
import json


PLACE_NAME = "IIT Kharagpur"


def export_graph():

    print(f"Downloading graph for {PLACE_NAME}...")

    # -----------------------------
    # DOWNLOAD UNPROJECTED GRAPH
    # -----------------------------
    G_unprojected = ox.graph_from_place(
        PLACE_NAME,
        network_type="walk",
        simplify=False,
        retain_all=True
    )

    # -----------------------------
    # PROJECT GRAPH
    # -----------------------------
    G_projected = ox.project_graph(G_unprojected)

    # -----------------------------
    # CREATE NODE ID MAPPING
    # OSM ID -> INTERNAL ID
    # -----------------------------
    node_id_map = {}

    for index, osm_id in enumerate(G_unprojected.nodes()):
        node_id_map[osm_id] = index

    # -----------------------------
    # EXPORT DATA STRUCTURE
    # -----------------------------
    graph_data = {
        "nodes": [],
        "edges": []
    }

    # -----------------------------
    # EXPORT NODES
    # -----------------------------
    for osm_id in G_unprojected.nodes():

        # original lat/lon
        original_data = G_unprojected.nodes[osm_id]

        # projected x/y
        projected_data = G_projected.nodes[osm_id]

        node = {
            "id": node_id_map[osm_id],
            "osm_id": int(osm_id),
            "lat": round(original_data["y"],6),
            "lon": round(original_data["x"],6),

            "x": round(projected_data["x"],2),
            "y": round(projected_data["y"],2)
        }

        graph_data["nodes"].append(node)

    # -----------------------------
    # EXPORT EDGES
    # -----------------------------
    for u, v, edge_data in G_unprojected.edges(data=True):

        edge = {
            "from": node_id_map[u],
            "to": node_id_map[v],

            "length": round(float(edge_data.get("length", 1.0)), 2)
        }

        graph_data["edges"].append(edge)

    # -----------------------------
    # SAVE JSON
    # -----------------------------
    output_path = "../data/outdoor/navigation/campus_graph.json"

    with open(output_path, "w") as f:
        json.dump(graph_data, f, indent=4)

    print("campus_graph.json created successfully")
    print()

    print("GRAPH VALIDATION")
    print("-" * 30)

    print("Total Nodes:", len(graph_data["nodes"]))
    print("Total Edges:", len(graph_data["edges"]))

    # -----------------------------
    # CHECK NODE IDS
    # -----------------------------
    node_ids = set()

    for node in graph_data["nodes"]:
        node_ids.add(node["id"])

    print("Unique Node IDs:", len(node_ids))

    # -----------------------------
    # CHECK EDGE REFERENCES
    # -----------------------------
    invalid_edges = 0

    for edge in graph_data["edges"]:

        if edge["from"] not in node_ids:
            invalid_edges += 1

        if edge["to"] not in node_ids:
            invalid_edges += 1

    print("Invalid Edge References:", invalid_edges)

    # -----------------------------
    # SAMPLE DATA
    # -----------------------------
    print()
    print("Sample Node:")
    print(graph_data["nodes"][0])

    print()
    print("Sample Edge:")
    print(graph_data["edges"][0])


if __name__ == "__main__":
    export_graph( ) 