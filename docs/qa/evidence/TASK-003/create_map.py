"""Run inside UE Editor through UEClient.runtime.launch_editor -ExecutePythonScript.

Creates only the reserved TASK-003 greybox assets; never overwrites an existing map.
"""
from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.project_dir())
out = root / "Saved" / "Task003"
out.mkdir(parents=True, exist_ok=True)
asset_root = "/Game/Hearthward/Bootstrap"
map_path = asset_root + "/L_Bootstrap"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def material(name, color):
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, asset_root, unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
    rough.set_editor_property("r", 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def box(name, pos, scale, mat):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*pos))
    actor.set_actor_label(name)
    component = actor.static_mesh_component
    component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    component.set_material(0, mat)
    component.set_collision_profile_name("BlockAll")
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


try:
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        raise RuntimeError("Bootstrap map already exists; refusing to overwrite")
    assert levels.new_level(map_path)
    floor = material("M_Floor", (0.23, 0.28, 0.31))
    wall = material("M_Wall", (0.46, 0.51, 0.54))
    marker = material("M_Marker", (0.68, 0.35, 0.12))
    box("Floor", (0, 0, -25), (40, 40, 0.5), floor)
    box("NorthBoundary", (2000, 0, 200), (0.5, 40, 4), wall)
    box("SouthBoundary", (-2000, 0, 200), (0.5, 40, 4), wall)
    box("EastBoundary", (0, 2000, 200), (40, 0.5, 4), wall)
    box("WestBoundary", (0, -2000, 200), (40, 0.5, 4), wall)
    box("CollisionWall", (1000, 0, 150), (1, 10, 3), marker)
    for y in (-1000, 1000):
        box("Landmark", (600, y, 100), (1.5, 1.5, 2), marker)
    for x in (-1000, -500, 0, 500):
        box("DistanceStripe", (x, 0, 0.5), (0.03, 30, 0.01), wall)
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-45, -35, 0))
    sun.light_component.set_editor_property("intensity", 3.0)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("intensity", 0.8)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-1200, -1200, 1100), unreal.Rotator(-30, 45, 0))
    assert levels.save_current_level()
    (out / "map-created.json").write_text(json.dumps({"ok": True, "map": map_path}), encoding="utf-8")
except Exception as exc:
    (out / "map-created.json").write_text(json.dumps({"ok": False, "error": str(exc)}), encoding="utf-8")
    raise
