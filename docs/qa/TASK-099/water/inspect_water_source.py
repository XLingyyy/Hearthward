"""Root-run read-only inspection of the exact existing western lake.

Load the saved ExternalActor package in the owned Bootstrap Editor; do not open
the natural map, start PIE, import, modify, save, spawn, or register any object.
Saved actor metadata never proves a currently active gameplay sound source.
"""

from datetime import datetime, timezone
import json
import math
from pathlib import Path
import struct
from uuid import uuid4

import unreal


MESH = "/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/Lake_0/StaticMeshes/SM_Lake_0"
EXTERNAL_ACTOR = "/Game/__ExternalActors__/Hearthward/World/Natural/Rebuild/L_HearthwardWilds/0/NK/VWCZK1XJDUUVFVUT770Z9F"
SOURCE = "art_source/TASK-026/Rebuild/water/Lake_0.glb"


def metadata(value):
    if value is None:
        return None
    return {"path": value.get_path_name(), "class": value.get_class().get_path_name()}


def vector(value):
    result = [float(value.x), float(value.y), float(value.z)]
    if not all(math.isfinite(component) for component in result):
        raise ValueError("Non-finite geometry/transform")
    return result


def transform(value):
    rotation = value.rotation
    return {
        "translation_cm": vector(value.translation),
        "rotation_quaternion_xyzw": [float(rotation.x), float(rotation.y), float(rotation.z), float(rotation.w)],
        "scale": vector(value.scale3d),
    }


def read(callback, conversion=lambda value: value):
    try:
        return {"state": "READ", "value": conversion(callback())}
    except Exception as error:
        return {"state": "NOT_READ", "error_kind": type(error).__name__, "error": str(error)}


def property_read(value, name, conversion=lambda item: item):
    return read(lambda: value.get_editor_property(name), conversion)


def package_name(value):
    package = value.get_package()
    return package.get_name() if package else None


def actor_world(actor):
    # AActor.GetLevel is reflected; UObject.GetWorld/HasActorBegunPlay and
    # UActorComponent.IsRegistered are not reflected in the inspected headers.
    value = actor.get_level()
    while value is not None and not isinstance(value, unreal.World):
        value = value.get_outer()
    return value


def current_worlds():
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    return subsystem.get_editor_world(), subsystem.get_game_world()


def candidates():
    rows = []
    for actor in unreal.ObjectIterator(unreal.Actor):
        if unreal.SystemLibrary.is_valid(actor):
            exact_package = package_name(actor) == EXTERNAL_ACTOR
            components = actor.get_components_by_class(unreal.StaticMeshComponent)
            matching = [component for component in components
                        if component.get_editor_property("static_mesh") is not None
                        and component.get_editor_property("static_mesh").get_path_name().split(".")[0] == MESH]
            if exact_package or matching:
                rows.append((actor, components if exact_package else matching))
    return rows


def inspect_component(component):
    return {
        "object": metadata(component),
        "mesh": property_read(component, "static_mesh", metadata),
        "world_transform": read(component.get_world_transform, transform),
        "relative_transform": read(component.get_relative_transform, transform),
        "registered": read(lambda: component.is_registered()),
        "registered_limit": "NOT_READ remains unknown; visibility/transform do not establish registration",
        "visible": property_read(component, "visible", bool),
        "hidden_in_game": property_read(component, "hidden_in_game", bool),
        "mobility": property_read(component, "mobility", str),
        "collision_profile": read(component.get_collision_profile_name, str),
        "collision_enabled": read(component.get_collision_enabled, str),
        "materials": read(component.get_materials, lambda values: [metadata(value) for value in values]),
    }


def inspect_actor(actor, components, before_paths, editor_world, game_world):
    world = read(lambda: actor_world(actor), metadata)
    actual_world = actor_world(actor)
    game_member = game_world is not None and actual_world == game_world
    return {
        "object": metadata(actor),
        "package": package_name(actor),
        "seen_before_exact_package_load": actor.get_path_name() in before_paths,
        "evidence_kind": "CURRENT_GAME_WORLD_OBJECT_ACTIVITY_UNCONFIRMED" if game_member else "SAVED_OR_EDITOR_OBJECT_ONLY",
        "active_gameplay_source": "NOT_PROVEN; registration/BeginPlay/streaming activity are separate gates",
        "world": world,
        "matches_current_editor_world": editor_world is not None and actual_world == editor_world,
        "matches_current_game_world": game_member,
        "level": read(actor.get_level, metadata),
        "level_transform": read(actor.get_level_transform, transform),
        "actor_transform": read(actor.get_actor_transform, transform),
        "tags": property_read(actor, "tags", lambda values: [str(value) for value in values]),
        "hidden_in_game": property_read(actor, "hidden", bool),
        "is_spatially_loaded": property_read(actor, "is_spatially_loaded", bool),
        "data_layer_assets": property_read(actor, "data_layer_assets", lambda values: [metadata(value) for value in values]),
        "actor_collision_enabled": read(actor.get_actor_enable_collision, bool),
        "actor_tick_enabled": read(actor.is_actor_tick_enabled, bool),
        "being_destroyed": read(actor.is_actor_being_destroyed, bool),
        "has_begun_play": read(lambda: actor.has_actor_begun_play(), bool),
        "components": [inspect_component(component) for component in components],
    }


def inspect_geometry(mesh):
    row = {"state": "NOT_READ", "coordinate_space": "Actual UE StaticMesh LOD0 local centimetres; not glTF coordinates"}
    try:
        description = mesh.get_static_mesh_description(0)
        if description is None:
            raise RuntimeError("No readable LOD0 StaticMeshDescription")
        vertex_count = int(description.get_vertex_count())
        triangle_count = int(description.get_triangle_count())
        row.update({"vertex_count": vertex_count, "triangle_count": triangle_count,
                    "vertices_state": "NOT_READ", "triangles_state": "NOT_READ"})
        vertices = []
        for index in range(vertex_count):
            vertex_id = unreal.VertexID(id_value=index)
            if not description.is_vertex_valid(vertex_id):
                raise RuntimeError("Sparse vertex IDs: cannot interpret count as maximum ID")
            vertices.append(vector(description.get_vertex_position(vertex_id)))
        row.update({"vertices_state": "READ", "vertices_local_cm": vertices})
        triangles = []
        if triangle_count:
            # UE5.8 GetTriangleVertices initializes three slots, then Algo::Copy
            # appends three. Read the length only; uninitialized IDs are not data.
            row["array_triangle_vertex_probe_length"] = len(description.get_triangle_vertices(unreal.TriangleID(id_value=0)))
        for index in range(triangle_count):
            triangle_id = unreal.TriangleID(id_value=index)
            if not description.is_triangle_valid(triangle_id):
                raise RuntimeError("Sparse triangle IDs: cannot interpret count as maximum ID")
            ids = []
            for corner in range(3):
                instance = description.get_triangle_vertex_instance(triangle_id, corner)
                if not description.is_vertex_instance_valid(instance):
                    raise RuntimeError("Invalid actual triangle vertex-instance ID")
                vertex = description.get_vertex_instance_vertex(instance)
                ids.append(int(vertex.get_editor_property("id_value")))
            if len(ids) != 3 or any(value < 0 or value >= vertex_count for value in ids):
                raise RuntimeError("Invalid actual triangle vertex IDs")
            triangles.append(ids)
        row.update({"state": "READ", "triangles_state": "READ",
                    "vertices_local_cm": vertices, "triangles": triangles,
                    "triangle_read_api": "GetTriangleVertexInstance(index0..2) -> GetVertexInstanceVertex; actual validated IDs",
                    "bounds_local_cm": {"min": [min(point[axis] for point in vertices) for axis in range(3)],
                                        "max": [max(point[axis] for point in vertices) for axis in range(3)]}})
    except Exception as error:
        row.update({"error_kind": type(error).__name__, "error": str(error)})
    return row


def inspect_source(project):
    row = {"state": "NOT_READ", "project_relative_source": SOURCE,
           "coordinate_space": "Retained glTF local axes/metres; not runtime UE geometry"}
    try:
        data = (project / SOURCE).read_bytes()
        if data[:4] != b"glTF" or struct.unpack_from("<I", data, 4)[0] != 2:
            raise ValueError("Expected retained glTF2 binary source")
        length, kind = struct.unpack_from("<II", data, 12)
        if kind != 0x4E4F534A:
            raise ValueError("Expected JSON chunk")
        source = json.loads(data[20:20+length])
        primitive = source["meshes"][0]["primitives"][0]
        positions = source["accessors"][primitive["attributes"]["POSITION"]]
        indices = source["accessors"][primitive["indices"]]
        if primitive.get("mode", 4) != 4 or indices["count"] % 3:
            raise ValueError("Expected actual triangle source")
        row.update({"state": "READ", "bytes": len(data), "vertex_count": positions["count"],
                    "triangle_count": indices["count"] // 3,
                    "bounds": {"min": positions.get("min"), "max": positions.get("max")},
                    "geometry_identity": "NOT_INFERRED; UE import may weld/reorder/transform vertices",
                    "licence": "UNKNOWN; import pairing and presence do not establish authorship/terms"})
    except Exception as error:
        row.update({"error_kind": type(error).__name__, "error": str(error)})
    return row


def inspect_import(mesh, project):
    def source_record(value):
        path = Path(value)
        if path.is_absolute():
            resolved = path.resolve()
            if resolved.is_relative_to(project / "art_source"):
                return {"project_relative_source": resolved.relative_to(project).as_posix(),
                        "present": resolved.is_file(), "bytes": resolved.stat().st_size if resolved.is_file() else None}
        return {"filename": path.name, "state": "NOT_READ_OUTSIDE_EXACT_PROJECT_SOURCE"}
    try:
        import_data = mesh.get_editor_property("asset_import_data")
        if import_data is None:
            return {"state": "NO_IMPORT_DATA", "files": None}
        return {"state": "READ", "object": metadata(import_data),
                "files": [source_record(value) for value in import_data.extract_filenames()],
                "licence_inference": "NONE"}
    except Exception as error:
        return {"state": "NOT_READ", "error_kind": type(error).__name__, "error": str(error)}


def main():
    project = Path(unreal.Paths.project_dir()).resolve()
    now = datetime.now(timezone.utc)
    run = "water-source-" + now.strftime("%Y%m%dT%H%M%SZ-") + uuid4().hex[:8]
    result = {"task": "TASK-099", "run_id": run, "created_utc": now.isoformat(),
              "engine_version": unreal.SystemLibrary.get_engine_version(),
              "expected_mesh_package": MESH, "expected_external_actor_package": EXTERNAL_ACTOR,
              "status": "PARTIAL_OR_FAILED", "asset_saves_requested": 0, "asset_setters_called": [],
              "map_open_requested": False, "pie_start_requested": False,
              "source_glb": inspect_source(project)}
    try:
        editor_world, game_world = current_worlds()
        result["worlds_before_package_load"] = {"editor": metadata(editor_world), "game": metadata(game_world)}
        before_paths = {actor.get_path_name() for actor, _ in candidates()}
        mesh = unreal.load_asset(MESH)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Exact mesh is missing or not StaticMesh")
        result["mesh"] = metadata(mesh)
        result["import_data"] = inspect_import(mesh, project)
        result["allow_cpu_access"] = property_read(mesh, "allow_cpu_access", bool)
        result["geometry"] = inspect_geometry(mesh)
        package = unreal.load_package(EXTERNAL_ACTOR)
        result["external_actor_package_load"] = {"state": "READ" if package is not None else "NOT_READ", "package": metadata(package)}
        after_editor, after_game = current_worlds()
        result["worlds_after_package_load"] = {"editor": metadata(after_editor), "game": metadata(after_game)}
        result["current_editor_world_changed"] = after_editor != editor_world
        result["actors"] = [inspect_actor(actor, components, before_paths, after_editor, after_game)
                            for actor, components in candidates()]
        if result["geometry"]["state"] == "READ" and result["actors"]:
            result["status"] = "READ_SAVED_METADATA_GAMEPLAY_ACTIVITY_NOT_PROVEN"
    except Exception as error:
        result["error_kind"] = type(error).__name__
        result["error"] = str(error)
    result["limits"] = [
        "Loading the exact package yields saved object evidence, not a loaded/active gameplay source.",
        "NOT_READ registration or BeginPlay is unknown; transforms/visibility do not replace it.",
        "Geometry read is current UE LOD0 local centimetres, distinct from retained source glTF axes/metres.",
        "Source counts 98/96 are a comparison datum; no assumed identity or vertices invented on API failure.",
        "No map opens, asset setters/saves, registration, spawning, collision changes, playback, traces or download.",
        "No acoustic radius, weather, wind/fire activity, swimming region or gameplay rules are inferred.",
    ]
    destination = project / ".agent-local" / "qa" / "TASK-099" / run
    destination.mkdir(parents=True, exist_ok=False)
    output = destination / "metadata.json"
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    unreal.log("TASK099_WATER_SOURCE_REPORT=" + str(output))


main()
