"""Root-only exact stone-axe geometric sockets; author and fresh-process probe."""
import json
from pathlib import Path
import traceback
import unreal

HERE = Path(__file__).resolve().parent
OUT = Path(unreal.Paths.project_saved_dir()) / "Task055"
ACTION = "author" if "-task055socketaction=author" in unreal.SystemLibrary.get_command_line().lower() else "probe"
DATA = json.loads((HERE / "stone-axe-measured-endpoints.json").read_text(encoding="utf-8-sig"))
PATH = "/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe"
SOCKETS = ("Grip", "BladeBase", "BladeTip")
REPORT = {"action": ACTION, "ok": False, "mesh": PATH, "saved_assets": [],
          "source": "Actual imported mesh coordinates and two actual cutting-edge vertices",
          "palm_alignment": "not accepted; Grip is only the measured geometric shaft centre"}

def xyz(value):
    return [value.x, value.y, value.z]

def sockets(mesh):
    result = {}
    for name in SOCKETS:
        socket = mesh.find_socket(name)
        result[name] = None if socket is None else {
            "name": str(socket.get_editor_property("socket_name")),
            "location_cm": xyz(socket.get_editor_property("relative_location")),
            "rotation_degrees": str(socket.get_editor_property("relative_rotation")),
            "scale": xyz(socket.get_editor_property("relative_scale")),
            "outer": socket.get_outer().get_path_name()}
    return result

try:
    if DATA["mesh_path"] != PATH:
        raise RuntimeError("Measured endpoint manifest points at another mesh")
    mesh = unreal.load_asset(PATH)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Actual existing stone axe is missing")
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    before_materials = [mesh.get_material(i).get_path_name() for i in range(len(mesh.get_editor_property("static_materials")))]
    before_collision = [subsystem.get_simple_collision_count(mesh), subsystem.get_convex_collision_count(mesh)]
    REPORT["before"] = sockets(mesh)
    description = mesh.get_static_mesh_description(0)
    if description is None:
        raise RuntimeError("Actual imported LOD0 source description missing")
    REPORT["actual_blade_vertices"] = {}
    for name, vertex in DATA["blade_vertices"].items():
        value = description.get_vertex_position(unreal.VertexID(id_value=vertex))
        expected = unreal.Vector(*DATA["candidates"][name])
        if (value - expected).length() > 0.00001:
            raise RuntimeError("Blade endpoint no longer matches its actual source vertex")
        REPORT["actual_blade_vertices"][name] = {"vertex_id": vertex, "local_cm": xyz(value)}
    if ACTION == "author":
        if any(REPORT["before"][name] is not None for name in SOCKETS):
            raise RuntimeError("Geometric sockets already exist; inspect before another author attempt")
        for name in SOCKETS:
            socket = unreal.StaticMeshSocket(outer=mesh, name="Task055_" + name)
            socket.set_editor_property("socket_name", name)
            socket.set_editor_property("relative_location", unreal.Vector(*DATA["candidates"][name]))
            socket.set_editor_property("relative_rotation", unreal.Rotator(0, 0, 0))
            socket.set_editor_property("relative_scale", unreal.Vector(1, 1, 1))
            mesh.add_socket(socket)
    REPORT["actual"] = sockets(mesh)
    for name in SOCKETS:
        socket = mesh.find_socket(name)
        if socket is None or socket.get_outer() != mesh or str(socket.get_editor_property("socket_name")) != name:
            raise RuntimeError("Actual socket identity/outer mismatch")
        if (socket.get_editor_property("relative_location") - unreal.Vector(*DATA["candidates"][name])).length() > 0.00001:
            raise RuntimeError("Actual socket coordinate mismatch")
        if (socket.get_editor_property("relative_scale") - unreal.Vector(1, 1, 1)).length() > 0.00001:
            raise RuntimeError("Unexpected socket scale")
    REPORT["materials"] = [mesh.get_material(i).get_path_name() for i in range(len(mesh.get_editor_property("static_materials")))]
    REPORT["collision_counts"] = [subsystem.get_simple_collision_count(mesh), subsystem.get_convex_collision_count(mesh)]
    if REPORT["materials"] != before_materials or REPORT["collision_counts"] != before_collision:
        raise RuntimeError("Socket author altered material/collision data")
    if ACTION == "author":
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=True):
            raise RuntimeError("Exact existing mesh package save failed")
        REPORT["saved_assets"] = [mesh.get_path_name()]
    REPORT["ok"] = True
except Exception:
    REPORT["error"] = traceback.format_exc()
OUT.mkdir(parents=True, exist_ok=True)
(OUT / ("stone-axe-sockets-" + ACTION + ".json")).write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.SystemLibrary.quit_editor()
