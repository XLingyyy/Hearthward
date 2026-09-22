"""Create the TASK-026 natural-world map and its task-owned materials.

The script is intentionally one-shot: it refuses to overwrite the level or any
reserved asset. It uses the UE 5.8 Open World template for World Partition,
scales the landscape footprint to 4.032 km, and batches repeated natural forms
with HISM components so the saved world has a small actor count.
"""

import json
import math
import random
import traceback
from collections import defaultdict
from pathlib import Path

import unreal


SEED = 260921
MAP_PATH = "/Game/Hearthward/World/Natural/L_NaturalWorld"
ASSET_ROOT = "/Game/Hearthward/Assets/NaturalWorld"
TEXTURE_ROOT = ASSET_ROOT + "/Textures"
MATERIAL_ROOT = ASSET_ROOT + "/Materials"
OPEN_WORLD_TEMPLATE = "/Engine/Maps/Templates/OpenWorld"

root = Path(unreal.Paths.project_dir())
output_dir = root / "Saved" / "Task026"
output_dir.mkdir(parents=True, exist_ok=True)
output_file = output_dir / "creation.json"

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
created_assets = []
created_actors = []
instance_counts = {}


def reserve_paths():
    return [
        MAP_PATH,
        TEXTURE_ROOT + "/T_GrassGround_D",
        TEXTURE_ROOT + "/T_Dirt_D",
        TEXTURE_ROOT + "/T_Rock_D",
        MATERIAL_ROOT + "/M_GroundGrass",
        MATERIAL_ROOT + "/M_Soil",
        MATERIAL_ROOT + "/M_Rock",
        MATERIAL_ROOT + "/M_Water",
        MATERIAL_ROOT + "/M_Foliage",
        MATERIAL_ROOT + "/M_Bark",
        MATERIAL_ROOT + "/M_Trail",
    ]


def import_texture(source, name):
    task = unreal.AssetImportTask()
    task.set_editor_properties(
        {
            "automated": True,
            "destination_name": name,
            "destination_path": TEXTURE_ROOT,
            "filename": str(source),
            "replace_existing": False,
            "save": True,
        }
    )
    asset_tools.import_asset_tasks([task])
    path = TEXTURE_ROOT + "/" + name
    texture = unreal.load_asset(path)
    assert texture, f"Texture import failed: {source}"
    texture.set_editor_property("sRGB", True)
    texture.set_editor_property("never_stream", False)
    texture.set_editor_property("lod_bias", 1)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    created_assets.append(path)
    return texture


def make_textured_material(name, texture, tiling, roughness=0.9):
    mat = asset_tools.create_asset(name, MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    assert mat
    uv = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionTextureCoordinate
    )
    uv.set_editor_property("u_tiling", tiling)
    uv.set_editor_property("v_tiling", tiling)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionTextureSample
    )
    sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", sample, "UVs")
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant
    )
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
    created_assets.append(mat.get_path_name())
    return mat


def make_color_material(name, color, roughness, opacity=None, emissive=0.0):
    mat = asset_tools.create_asset(name, MATERIAL_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    assert mat
    base = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector
    )
    base.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        mat, unreal.MaterialExpressionConstant
    )
    rough.set_editor_property("r", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    if emissive:
        strength = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionMultiply
        )
        scalar = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant
        )
        scalar.set_editor_property("r", emissive)
        unreal.MaterialEditingLibrary.connect_material_expressions(base, "", strength, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(scalar, "", strength, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            strength, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
    if opacity is not None:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        alpha = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant
        )
        alpha.set_editor_property("r", opacity)
        unreal.MaterialEditingLibrary.connect_material_property(
            alpha, "", unreal.MaterialProperty.MP_OPACITY
        )
    unreal.MaterialEditingLibrary.recompile_material(mat)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
    created_assets.append(mat.get_path_name())
    return mat


def transform(x, y, z, sx, sy, sz, yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Transform(
        location=unreal.Vector(x, y, z),
        rotation=unreal.Rotator(pitch, yaw, roll),
        scale=unreal.Vector(sx, sy, sz),
    )


def quadrant(x, y):
    return ("N" if y >= 0 else "S") + ("E" if x >= 0 else "W")


def make_hism_actor(label, mesh, material, transforms, collision, folder, cull_distance):
    if not transforms:
        return None
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector())
    assert actor
    actor.set_actor_label(label)
    actor.tags = ["TASK026.NATURAL_WORLD", label]
    actor.set_folder_path(folder)
    actor.set_editor_property("is_spatially_loaded", True)
    component = unreal.new_object(
        unreal.HierarchicalInstancedStaticMeshComponent,
        outer=actor,
        name=label + "_HISM",
    )
    component.set_static_mesh(mesh)
    component.set_material(0, material)
    component.set_collision_profile_name(collision)
    component.set_cull_distances(0, cull_distance)
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    component.set_editor_property("cast_shadow", label not in {"Grass", "Trail", "Water"})
    actor.set_editor_property("root_component", component)
    for item in transforms:
        component.add_instance(item)
    count = component.get_instance_count()
    assert count == len(transforms), f"Instance count mismatch for {label}"
    created_actors.append(label)
    instance_counts[label] = count
    return actor


def distance_to_segment(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    if dx == 0 and dy == 0:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def river_x(y):
    return 11000.0 * math.sin(y / 42000.0) + 4500.0 * math.sin(y / 18000.0)


def is_ford(y):
    return abs(y + 50000.0) < 10000.0 or abs(y - 50000.0) < 10000.0


def on_route(x, y, clearance=2800.0):
    corners = [(-70000, -50000), (70000, -50000), (70000, 50000), (-70000, 50000)]
    for index, start in enumerate(corners):
        end = corners[(index + 1) % len(corners)]
        if distance_to_segment(x, y, *start, *end) < clearance:
            return True
    return False


def in_clearing(x, y):
    return (
        math.hypot(x + 85000, y + 65000) < 22000
        or math.hypot(x - 82000, y - 62000) < 24000
    )


def random_point(rng, x0, x1, y0, y1):
    return rng.uniform(x0, x1), rng.uniform(y0, y1)


try:
    conflicts = [path for path in reserve_paths() if unreal.EditorAssetLibrary.does_asset_exist(path)]
    assert not conflicts, f"Reserved assets already exist; refusing to overwrite: {conflicts}"

    assert levels.new_level_from_template(MAP_PATH, OPEN_WORLD_TEMPLATE)

    # The stock template has 2017 samples at 1 m spacing. Double only XY scale
    # and proxy XY locations to produce a 4032 m footprint while preserving Z.
    template_actor_count = 0
    landscape_proxy_count = 0
    for actor in actors.get_all_level_actors():
        template_actor_count += 1
        class_name = actor.get_class().get_name()
        if class_name in {"Landscape", "LandscapeStreamingProxy"}:
            location = actor.get_actor_location()
            scale = actor.get_actor_scale3d()
            actor.set_actor_location(unreal.Vector(location.x * 2.0, location.y * 2.0, location.z), False, False)
            actor.set_actor_scale3d(unreal.Vector(scale.x * 2.0, scale.y * 2.0, scale.z))
            if class_name == "LandscapeStreamingProxy":
                landscape_proxy_count += 1
        elif class_name == "PlayerStart":
            actor.set_actor_location(unreal.Vector(-70000, -50000, 6200), False, False)
            actor.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
            actor.set_actor_label("PlayerStart_NaturalWorld")

    # Reuse only the existing source textures; imported derivatives are owned by 026.
    grass_texture = import_texture(
        root / "art_source/TASK-004/polyhaven/地表/草地/textures/grass_ground_diff_4k.jpg",
        "T_GrassGround_D",
    )
    dirt_texture = import_texture(
        root / "art_source/TASK-004/polyhaven/地表/土/textures/dirt_diff_4k.jpg",
        "T_Dirt_D",
    )
    rock_texture = import_texture(
        root / "art_source/TASK-004/polyhaven/地表/岩面/textures/rocky_terrain_diff_4k.jpg",
        "T_Rock_D",
    )
    grass_mat = make_textured_material("M_GroundGrass", grass_texture, 36.0, 0.96)
    soil_mat = make_textured_material("M_Soil", dirt_texture, 20.0, 0.94)
    rock_mat = make_textured_material("M_Rock", rock_texture, 8.0, 0.88)
    water_mat = make_color_material("M_Water", (0.018, 0.15, 0.22), 0.16, 0.76, 0.09)
    foliage_mat = make_color_material("M_Foliage", (0.035, 0.16, 0.045), 0.92)
    bark_mat = make_color_material("M_Bark", (0.11, 0.055, 0.025), 0.98)
    trail_mat = make_textured_material("M_Trail", dirt_texture, 5.0, 0.97)

    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    sphere = unreal.load_asset("/Engine/BasicShapes/Sphere")
    cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder")
    cone = unreal.load_asset("/Engine/BasicShapes/Cone")
    assert cube and sphere and cylinder and cone

    batches = defaultdict(list)
    tile_size = 25000.0
    tile_half = tile_size / 2.0
    for ix in range(-6, 6):
        for iy in range(-6, 6):
            x = (ix + 0.5) * tile_size
            y = (iy + 0.5) * tile_size
            q = quadrant(x, y)
            center_distance = abs(x - river_x(y))
            if center_distance < 17000 and not is_ford(y):
                # Recessed riverbed; the water surface is visibly below the banks.
                batches[("Riverbed_" + q, cube, soil_mat, "BlockAll", "Natural/River", 220000)].append(
                    transform(x, y, 4200, tile_size / 100.0, tile_size / 100.0, 16.0)
                )
                water_width = 18000.0 if y < 75000 else 36000.0
                batches[("Water_" + q, cube, water_mat, "NoCollision", "Natural/Water", 250000)].append(
                    transform(river_x(y), y, 4700, water_width / 100.0, tile_size / 100.0, 2.0)
                )
            else:
                material = grass_mat if x < 65000 else (soil_mat if y < 25000 else rock_mat)
                batches[("Terrain_" + q + "_" + material.get_name(), cube, material, "BlockAll", "Natural/Terrain", 350000)].append(
                    transform(x, y, 4500, tile_size / 100.0, tile_size / 100.0, 10.0)
                )

    # Two broad, walkable fords at the loop crossings.
    for ford_y in (-50000.0, 50000.0):
        batches[("Fords", cube, rock_mat, "BlockAll", "Natural/River", 220000)].append(
            transform(river_x(ford_y), ford_y, 4800, 360.0, 180.0, 4.0)
        )

    # Worn natural trail: 4.8 km main loop plus two branches.
    route_segments = []
    for x in range(-70000, 70001, 10000):
        route_segments.append(transform(x, -50000, 5020, 90.0, 8.0, 0.3))
        route_segments.append(transform(x, 50000, 5020, 90.0, 8.0, 0.3))
    for y in range(-40000, 40001, 10000):
        route_segments.append(transform(-70000, y, 5020, 8.0, 90.0, 0.3))
        route_segments.append(transform(70000, y, 5020, 8.0, 90.0, 0.3))
    for y in range(60000, 120001, 10000):
        route_segments.append(transform(0, y, 5020, 8.0, 90.0, 0.3))
    for x in range(80000, 130001, 10000):
        route_segments.append(transform(x, 0, 5020, 90.0, 8.0, 0.3))
    for item in route_segments:
        location = item.translation
        batches[("Trail_" + quadrant(location.x, location.y), cube, trail_mat, "NoCollision", "Natural/Routes", 180000)].append(item)

    rng = random.Random(SEED)
    trunks = defaultdict(list)
    crowns = defaultdict(list)
    shrubs = defaultdict(list)
    grass_tufts = defaultdict(list)
    rocks = defaultdict(list)

    # Dense western woodland with a clear trail and two preserved future sites.
    while sum(len(items) for items in trunks.values()) < 760:
        x, y = random_point(rng, -145000, -25000, -120000, 140000)
        if abs(x - river_x(y)) < 26000 or on_route(x, y) or in_clearing(x, y):
            continue
        q = quadrant(x, y)
        height = rng.uniform(900, 1700)
        radius = rng.uniform(55, 95)
        trunks[q].append(transform(x, y, 5000 + height / 2, radius / 50, radius / 50, height / 50, rng.uniform(0, 360)))
        crown_z = 5000 + height + rng.uniform(220, 420)
        crowns[q].append(transform(x, y, crown_z, rng.uniform(2.2, 4.2), rng.uniform(2.2, 4.2), rng.uniform(2.4, 4.8), rng.uniform(0, 360)))

    while sum(len(items) for items in shrubs.values()) < 900:
        x, y = random_point(rng, -145000, 130000, -140000, 140000)
        if abs(x - river_x(y)) < 20000 or on_route(x, y, 1800) or in_clearing(x, y):
            continue
        q = quadrant(x, y)
        shrubs[q].append(transform(x, y, 5200, rng.uniform(0.6, 1.4), rng.uniform(0.6, 1.4), rng.uniform(0.35, 0.8), rng.uniform(0, 360)))

    while sum(len(items) for items in grass_tufts.values()) < 3200:
        x, y = random_point(rng, -145000, 145000, -140000, 140000)
        if abs(x - river_x(y)) < 19000 or on_route(x, y, 1100) or in_clearing(x, y):
            continue
        q = quadrant(x, y)
        grass_tufts[q].append(transform(x, y, 5100, rng.uniform(0.25, 0.65), rng.uniform(0.25, 0.65), rng.uniform(0.7, 1.7), rng.uniform(0, 360)))

    while sum(len(items) for items in rocks.values()) < 420:
        x, y = random_point(rng, 25000, 145000, -135000, 135000)
        if abs(x - river_x(y)) < 21000 or on_route(x, y, 1500) or in_clearing(x, y):
            continue
        q = quadrant(x, y)
        rocks[q].append(transform(x, y, 5200, rng.uniform(0.8, 3.0), rng.uniform(0.7, 2.5), rng.uniform(0.5, 2.2), rng.uniform(0, 360), rng.uniform(-12, 12), rng.uniform(-12, 12)))

    for q, items in trunks.items():
        make_hism_actor("TreeTrunks_" + q, cylinder, bark_mat, items, "BlockAll", "Natural/Forest", 280000)
    for q, items in crowns.items():
        make_hism_actor("TreeCrowns_" + q, sphere, foliage_mat, items, "NoCollision", "Natural/Forest", 280000)
    for q, items in shrubs.items():
        make_hism_actor("Shrubs_" + q, sphere, foliage_mat, items, "NoCollision", "Natural/Vegetation", 160000)
    for q, items in grass_tufts.items():
        make_hism_actor("Grass_" + q, cone, foliage_mat, items, "NoCollision", "Natural/Vegetation", 90000)
    for q, items in rocks.items():
        make_hism_actor("Rocks_" + q, sphere, rock_mat, items, "BlockAll", "Natural/Rocks", 240000)

    # Three readable natural landmarks; all are outside the main trail clearance.
    landmark_batches = {
        "Landmark_SentinelRidge": [
            transform(-118000, 98000, 11500, 16, 16, 120, 5),
            transform(-103000, 108000, 9600, 13, 13, 88, -12),
            transform(-132000, 112000, 8200, 11, 11, 64, 20),
        ],
        "Landmark_TwinFangs": [
            transform(112000, -72000, 10000, 15, 15, 96, 8),
            transform(128000, -62000, 8800, 12, 12, 72, -16),
        ],
        "Landmark_RiverCrown": [
            transform(-9000, 112000, 7800, 9, 9, 52, 0),
            transform(10000, 118000, 8400, 10, 10, 64, 20),
            transform(27000, 110000, 7600, 8, 8, 48, -15),
        ],
    }
    for label, items in landmark_batches.items():
        make_hism_actor(label, cone, rock_mat, items, "BlockAll", "Natural/Landmarks", 400000)

    for key, items in batches.items():
        label, mesh, material, collision, folder, cull = key
        make_hism_actor(label, mesh, material, items, collision, folder, cull)

    unreal.EditorLevelLibrary.set_level_viewport_camera_info(
        unreal.Vector(-205000, -210000, 185000), unreal.Rotator(-28, 42, 0)
    )
    assert levels.save_current_level()
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    result = {
        "ok": True,
        "map": MAP_PATH,
        "seed": SEED,
        "world_partition_template": OPEN_WORLD_TEMPLATE,
        "landscape_samples": [2017, 2017],
        "landscape_xy_spacing_m": 2.0,
        "landscape_side_m": 4032,
        "central_walkable_base_km2": 9.0,
        "main_loop_m": 4800,
        "landscape_streaming_proxies": landscape_proxy_count,
        "template_actor_count": template_actor_count,
        "created_actor_count": len(created_actors),
        "instance_counts": instance_counts,
        "created_assets": created_assets + [MAP_PATH],
        "candidate_clearings": [
            {"id": "CAMP_A", "center_cm": [-85000, -65000, 5000], "clear_radius_m": 220},
            {"id": "HOMELAND_B", "center_cm": [82000, 62000, 5000], "clear_radius_m": 240},
        ],
        "fords": [
            {"center_cm": [river_x(-50000), -50000, 5000], "width_m": 36},
            {"center_cm": [river_x(50000), 50000, 5000], "width_m": 36},
        ],
        "landmarks": list(landmark_batches),
    }
except Exception:
    result = {
        "ok": False,
        "created_assets": created_assets,
        "created_actors": created_actors,
        "error": traceback.format_exc(),
    }

output_file.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log(f"TASK026_CREATION_RESULT={result.get('ok')}")
if not result["ok"]:
    raise RuntimeError(result["error"])
