"""Public, read-only UE generator for actual stone-axe source topology.

Call ``yield from export_handle_topology(axe_asset, OUT)`` in the existing
root-owned Slate generator. Does not start an editor or save/change any asset.
All connected faces are included when any corner is below the measured blade
base, so crossing faces at the region boundary are retained.
"""
import json
from pathlib import Path
import unreal


def export_handle_topology(axe_asset, output_directory):
    description = axe_asset.get_static_mesh_description(0)
    if description is None:
        raise RuntimeError("Actual stone axe LOD0 source MeshDescription missing")
    points = {}
    triangle_ids = set()
    selection_max_z = 10.305906295776367  # Actual authored BladeBase vertex Z.
    for index in range(description.get_vertex_count()):
        vertex_id = unreal.VertexID(id_value=index)
        if not description.is_vertex_valid(vertex_id):
            raise RuntimeError("Actual axe source vertex IDs are sparse")
        point = description.get_vertex_position(vertex_id)
        points[index] = [float(point.x), float(point.y), float(point.z)]
        if point.z <= selection_max_z:
            triangle_ids.update(int(t.id_value) for t in description.get_vertex_connected_triangles(vertex_id))
        if (index+1) % 2000 == 0:
            yield lambda: True
    triangles = []
    used_vertex_ids = set()
    api_diagnostic = None
    for index, triangle_index in enumerate(sorted(triangle_ids)):
        triangle_id = unreal.TriangleID(id_value=triangle_index)
        if not description.is_triangle_valid(triangle_id):
            raise RuntimeError("Connected axe triangle ID is invalid")
        instances = description.get_triangle_vertex_instances(triangle_id)
        vertices = [int(description.get_vertex_instance_vertex(v).id_value) for v in instances]
        if index == 0:
            old_result = description.get_triangle_vertices(triangle_id)
            old_ids = [int(v.id_value) for v in old_result]
            indexed_ids = [int(description.get_vertex_instance_vertex(description.get_triangle_vertex_instance(triangle_id, corner)).id_value) for corner in range(3)]
            api_diagnostic = {"triangle_id": triangle_index,
                              "old_get_triangle_vertices_python_type": str(type(old_result)),
                              "old_get_triangle_vertices_length": len(old_ids),
                              "old_get_triangle_vertices_actual_ids": old_ids,
                              "actual_vertex_instance_ids": [int(v.id_value) for v in instances],
                              "actual_mapped_vertex_ids": vertices,
                              "actual_indexed_corner_vertex_ids": indexed_ids,
                              "mapped_and_indexed_agree": vertices == indexed_ids,
                              "source_predicted_appended_tail_agrees": len(old_ids) == 6 and old_ids[3:] == vertices,
                              "method": "Actual public GetTriangleVertexInstances mapped through GetVertexInstanceVertex; old API inspected for diagnosis only, no old values used as geometry",
                              "source_evidence": "UE5.8 MeshDescriptionBase.cpp:756 SetNumUninitialized(3),757 Algo::Copy appends via Algo/Copy.h:40 Output.Add; first three old values are not valid geometry"}
            (Path(output_directory) / "stone-axe-handle-topology-api.json").write_text(json.dumps(api_diagnostic, indent=2), encoding="utf-8")
            if vertices != indexed_ids:
                raise RuntimeError("Actual triangle corner APIs disagree; diagnostic saved")
        if len(vertices) != 3:
            raise RuntimeError("Actual axe triangle has a non-three corner result")
        if any(vertex not in points for vertex in vertices):
            raise RuntimeError("Actual axe triangle references a vertex absent from actual source points")
        triangles.append([triangle_index] + vertices)
        used_vertex_ids.update(vertices)
        if (index+1) % 2000 == 0:
            yield lambda: True
    sockets = {}
    for name in ("Grip", "BladeBase", "BladeTip"):
        socket = axe_asset.find_socket(name)
        if socket is None:
            raise RuntimeError("Actual authored axe socket missing: " + name)
        p = socket.get_editor_property("relative_location")
        sockets[name] = [float(p.x), float(p.y), float(p.z)]
    output = {"mesh": {"path": axe_asset.get_path_name(), "class": axe_asset.get_class().get_name()},
              "coordinate_space": "UE mesh local centimetres, actual LOD0 source MeshDescription",
              "selection": "Connected triangles of every actual vertex with local Z <= actual measured BladeBase Z; boundary triangles retained",
              "selection_max_z_cm": selection_max_z,
              "actual_source_vertex_count": description.get_vertex_count(),
              "actual_source_triangle_count": description.get_triangle_count(),
              "triangle_vertex_api": "GetTriangleVertexInstances -> GetVertexInstanceVertex",
              "first_triangle_api_diagnostic": api_diagnostic,
              "selected_triangle_count": len(triangles), "actual_socket_relative_locations_cm": sockets,
              "vertices_id_xyz": [[i]+points[i] for i in sorted(used_vertex_ids)],
              "triangles_id_vertex_ids": triangles,
              "asset_mutations": [], "limits": ["Source topology is not a collision-hull proxy; skin subset still has open boundaries.", "No triangle surface contact or rendered grip acceptance is implied by this export."]}
    target = Path(output_directory) / "stone-axe-handle-ue-local-topology.json"
    target.write_text(json.dumps(output, separators=(",", ":")), encoding="utf-8")
    return str(target)
