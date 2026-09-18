"""Editor-only, one-shot creation of the isolated TASK-014 validation map."""
import json
import math
import traceback
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
out = root / "Saved/Task014"
out.mkdir(parents=True, exist_ok=True)
art = "/Game/Hearthward/Art/Graybox"
map_path = "/Game/Hearthward/Tests/Graybox/L_GrayboxValidation"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
created = []


def material(name, color):
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, art, unreal.Material, unreal.MaterialFactoryNew())
    rgb = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
    rgb.set_editor_property("constant", unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
    rough.set_editor_property("r", .85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
    created.append(mat.get_path_name())
    return mat


def box(name, position, size, mat, pitch=0, collision=True):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position), unreal.Rotator(pitch=pitch, yaw=0, roll=0))
    actor.set_actor_label(name)
    actor.tags = ["TASK014.PROTOTYPE_ONLY", name]
    actor.static_mesh_component.set_static_mesh(block)
    actor.static_mesh_component.set_material(0, mat)
    actor.static_mesh_component.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    actor.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    return actor


def sign(name, text, position, size=65):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*position), unreal.Rotator(pitch=0, yaw=180, roll=0))
    actor.set_actor_label(name)
    actor.tags = ["TASK014.PROTOTYPE_ONLY", name]
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(text)
    component.set_world_size(size)
    component.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    component.set_text_render_color(unreal.Color(240, 235, 210, 255))


try:
    reserved = [map_path] + [art + "/" + n for n in ["SM_Block100", "M_Ground", "M_Wall", "M_Accent", "M_Route"]]
    assert not any(unreal.EditorAssetLibrary.does_asset_exist(p) for p in reserved), "Reserved asset exists; refusing to overwrite"
    assert levels.new_level(map_path)
    ground = material("M_Ground", (.19, .23, .23))
    wall = material("M_Wall", (.40, .45, .44))
    accent = material("M_Accent", (.55, .25, .07))
    route = material("M_Route", (.62, .64, .51))
    block = unreal.EditorAssetLibrary.duplicate_asset("/Engine/BasicShapes/Cube", art + "/SM_Block100")
    assert block
    block.set_material(0, wall)
    assert unreal.EditorAssetLibrary.save_loaded_asset(block)
    created.append(block.get_path_name())
    box("Ground", (2500, 0, -25), (6000, 4000, 50), ground)
    for name, pos, size in [
        ("BoundaryWest", (-500, 0, 200), (50, 4000, 400)),
        ("BoundaryEast", (5500, 0, 200), (50, 4000, 400)),
        ("BoundaryNorth", (2500, 2000, 200), (6000, 50, 400)),
        ("BoundarySouth", (2500, -2000, 200), (6000, 50, 400)),
    ]:
        box(name, pos, size, wall)
    for x in range(0, 3501, 500):
        box("Stripe" + str(x), (x, 0, 1), (5, 700, 2), accent if x == 3000 else route, collision=False)
        if x:
            sign("Distance" + str(x), str(x // 100) + " m", (x, 440, 110), 45)
    box("CollisionStop", (3600, 0, 150), (100, 800, 300), accent)
    box("Ramp", (1200, -1100, 90), (math.hypot(800, 200), 600, 20), wall, math.degrees(math.atan2(200, 800)))
    box("RaisedPlatform", (1900, -1100, 175), (600, 600, 50), wall)
    # The gate is on the ground beyond the raised test area, with a 200 cm clear opening.
    box("GateLeft", (3100, -1550, 150), (100, 300, 300), wall)
    box("GateRight", (3100, -1050, 150), (100, 300, 300), wall)
    box("GateLintel", (3100, -1300, 275), (100, 200, 50), accent)
    sign("RampLabel", "02 / RAMP", (2000, -1700, 400), 60)
    sign("GateLabel", "03 / GATE", (3250, -1300, 420), 60)
    box("LandmarkBase", (4200, 1250, 50), (400, 400, 100), wall)
    box("LandmarkTower", (4200, 1250, 500), (170, 170, 800), accent)
    box("LandmarkCrown", (4200, 1250, 920), (300, 300, 40), route)
    sign("Title", "TASK 014 / GRAYBOX", (3900, 850, 620), 85)
    sign("ScopeLabel", "PROTOTYPE ONLY", (3900, 850, 510), 60)
    sign("LaneLabel", "01 / DISTANCE + COLLISION", (3500, 0, 430), 60)
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100), unreal.Rotator(pitch=0, yaw=0, roll=0))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(pitch=-50, yaw=-35, roll=0))
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("intensity", 3.0)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("intensity", .8)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-300, -1800, 1800), unreal.Rotator(pitch=-22, yaw=20, roll=0))
    assert levels.save_current_level()
    result = {"ok": True, "map": map_path, "created_assets": created + [map_path], "prototype_only": True}
except Exception:
    result = {"ok": False, "created_assets": created, "error": traceback.format_exc()}
(out / "creation.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
if not result["ok"]:
    raise RuntimeError(result["error"])
