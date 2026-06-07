import json
import plotly.graph_objects as go


def interactive_graph():

    # --------------------------------
    # LOAD GRAPH JSON
    # --------------------------------
    with open("data/outdoor/campus_graph.json", "r") as f:
        graph_data = json.load(f)

    # --------------------------------
    # LOAD BUILDINGS JSON
    # --------------------------------
    with open("data/outdoor/campus_buildings.json", "r") as f:
        building_data = json.load(f)

    nodes = graph_data["nodes"]
    edges = graph_data["edges"]

    print("Nodes Loaded:", len(nodes))
    print("Edges Loaded:", len(edges))
    print("Buildings Loaded:", len(building_data))

    # --------------------------------
    # CREATE NODE LOOKUP
    # --------------------------------
    node_lookup = {}

    for node in nodes:
        node_lookup[node["id"]] = node

    # --------------------------------
    # EDGE COORDINATES
    # --------------------------------
    edge_x = []
    edge_y = []

    for edge in edges:

        from_node = node_lookup[edge["from"]]
        to_node = node_lookup[edge["to"]]

        edge_x.extend([
            from_node["lon"],
            to_node["lon"],
            None
        ])

        edge_y.extend([
            from_node["lat"],
            to_node["lat"],
            None
        ])

    # --------------------------------
    # NODE COORDINATES
    # --------------------------------
    node_x = []
    node_y = []

    for node in nodes:

        node_x.append(node["lon"])
        node_y.append(node["lat"])

    # --------------------------------
    # BUILDING COORDINATES
    # --------------------------------
    building_x = []
    building_y = []
    building_text = []

    for building in building_data:

        building_x.append(building["lon"])
        building_y.append(building["lat"])

        building_text.append(
            f"""
            {building["name"]}
            <br>ID: {building["id"]}
            <br>Node ID: {building["node_id"]}
            """
        )

    # --------------------------------
    # EDGE TRACE
    # --------------------------------
    edge_trace = go.Scattergl(
        x=edge_x,
        y=edge_y,

        mode="lines",

        line=dict(
            width=1,
            color="white"
        ),

        hoverinfo="none",

        name="Roads"
    )

    # --------------------------------
    # NODE TRACE
    # --------------------------------
    node_trace = go.Scattergl(
        x=node_x,
        y=node_y,

        mode="markers",

        marker=dict(
            size=2,
            color="red"
        ),

        hoverinfo="none",

        name="Graph Nodes"
    )

    # --------------------------------
    # BUILDING TRACE
    # --------------------------------
    building_trace = go.Scattergl(
        x=building_x,
        y=building_y,

        mode="markers",

        marker=dict(
            size=8,
            color="cyan"
        ),

        text=building_text,

        hoverinfo="text",

        name="Buildings"
    )

    # --------------------------------
    # CREATE FIGURE
    # --------------------------------
    fig = go.Figure(
        data=[
            edge_trace,
            node_trace,
            building_trace
        ]
    )

    fig.update_layout(
        title="IIT Kharagpur Navigation Graph",

        showlegend=True,

        plot_bgcolor="black",
        paper_bgcolor="black",

        margin=dict(
            l=0,
            r=0,
            t=40,
            b=0
        ),

        xaxis=dict(
            showgrid=False,
            zeroline=False
        ),

        yaxis=dict(
            showgrid=False,
            zeroline=False,
            scaleanchor="x",
            scaleratio=1
        )
    )

    fig.show()


if __name__ == "__main__":
    interactive_graph()