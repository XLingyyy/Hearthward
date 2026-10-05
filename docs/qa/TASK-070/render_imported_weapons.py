"""Root-run only: capture the two imported weapons without changing saved assets.

Run in a freshly launched dedicated QA editor with a real RHI. NewBlankMap(False)
discards the current editor scene. Root owns process closure and image review.
"""
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import sys
import time
import traceback

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from probe_imported_weapons import mesh_info, object_info, rotation, vector

HERE = Path(__file__).resolve().parent
MANIFEST = json.loads((HERE / "first-weapon-import-manifest.json").read_text(encoding="utf-8-sig"))
match = re.search(r"-Task070RenderLabel=([A-Za-z0-9_-]+)", unreal.SystemLibrary.get_command_line())
LABEL = match.group(1) if match else "import-red"
OUT = Path(unreal.Paths.project_saved_dir()) / "Task070" / ("weapon-render-" + LABEL)
OUT.mkdir(parents=True, exist_ok=True)
REPORT = {
    "captured_utc": datetime.now(timezone.utc).isoformat(),
    "engine_version": unreal.SystemLibrary.get_engine_version(),
    "command_line": unreal.SystemLibrary.get_command_line(),
    "project_dir": unreal.Paths.project_dir(),
    "method": "Dedicated unsaved editor world; real SceneCapture2D lights and original mesh slot materials; capture_scene then next Slate tick PNG export",
    "label": LABEL,
    "capture_complete": False,
    "asset_mutations": [],
    "pieces": [],
    "acceptance": {"Owner_style": "pending", "hand_grip": "not measured", "gameplay_binding": "not measured", "combat_sweep": "not measured"},
}
STAGE = "setup"
REFERENCES = []


def after(seconds):
    end = time.monotonic() + seconds
    return lambda: time.monotonic() >= end


def fixture_actor(actors, actor_class, label, location=unreal.Vector(), rotation_value=unreal.Rotator()):
    actor = actors.spawn_actor_from_class(actor_class, location, rotation_value)
    if actor is None:
        raise RuntimeError("Failed to spawn fixture " + label)
    actor.set_actor_label("Task070_" + label)
    actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    REFERENCES.append(actor)
    return actor


def run():
    global STAGE
    if "-nullrhi" in unreal.SystemLibrary.get_command_line().lower():
        raise RuntimeError("Weapon render requires a real RHI")
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        raise RuntimeError("NewBlankMap(False) returned no editor world")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    REPORT["world"] = object_info(world)
    REPORT["world_saved"] = False
    REPORT["lights"] = []
    for label, light_rotation, intensity in (
        ("key", unreal.Rotator(pitch=-35, yaw=-45, roll=0), 30.0),
        ("fill", unreal.Rotator(pitch=-20, yaw=135, roll=0), 10.0),
    ):
        actor = fixture_actor(actors, unreal.DirectionalLight, label, rotation_value=light_rotation)
        component = actor.get_component_by_class(unreal.DirectionalLightComponent)
        component.set_intensity(intensity)
        REPORT["lights"].append({"actor": object_info(actor), "class": component.get_class().get_name(), "rotation": rotation(actor.get_actor_rotation()), "intensity": component.get_editor_property("intensity")})
    sky_actor = fixture_actor(actors, unreal.SkyLight, "sky")
    sky = sky_actor.get_component_by_class(unreal.SkyLightComponent)
    cubemap = unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap")
    if not isinstance(cubemap, unreal.TextureCube):
        raise RuntimeError("Fixture HDR cubemap is missing or not TextureCube")
    sky.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.set_cubemap(cubemap)
    sky.set_intensity(1.0)
    REPORT["sky_light"] = {"actor": object_info(sky_actor), "source_type": str(sky.get_editor_property("source_type")), "cubemap": object_info(sky.get_editor_property("cubemap")), "intensity": sky.get_editor_property("intensity")}
    capture_actor = fixture_actor(actors, unreal.SceneCapture2D, "capture")
    capture = capture_actor.capture_component2d
    target = unreal.RenderingLibrary.create_render_target2d(world, 1024, 1024, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    REFERENCES.append(target)
    capture.set_editor_property("texture_target", target)
    capture.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    capture.set_editor_property("projection_type", unreal.CameraProjectionMode.ORTHOGRAPHIC)
    capture.set_editor_property("capture_every_frame", False)
    capture.set_editor_property("capture_on_movement", False)
    settings = capture.get_editor_property("post_process_settings")
    settings.set_editor_property("override_auto_exposure_method", True)
    settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    settings.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
    settings.set_editor_property("auto_exposure_apply_physical_camera_exposure", True)
    settings.set_editor_property("override_depth_of_field_fstop", True)
    settings.set_editor_property("depth_of_field_fstop", 4.0)
    settings.set_editor_property("override_camera_shutter_speed", True)
    settings.set_editor_property("camera_shutter_speed", 64.0)
    settings.set_editor_property("override_camera_iso", True)
    settings.set_editor_property("camera_iso", 100.0)
    settings.set_editor_property("override_auto_exposure_bias", True)
    settings.set_editor_property("auto_exposure_bias", 0.0)
    settings.set_editor_property("override_bloom_intensity", True)
    settings.set_editor_property("bloom_intensity", 0.0)
    settings.set_editor_property("override_motion_blur_amount", True)
    settings.set_editor_property("motion_blur_amount", 0.0)
    capture.set_editor_property("post_process_settings", settings)
    actual = capture.get_editor_property("post_process_settings")
    REPORT["exposure"] = {
        "method": str(actual.get_editor_property("auto_exposure_method")),
        "apply_physical_camera_exposure": bool(actual.get_editor_property("auto_exposure_apply_physical_camera_exposure")),
        "aperture_fstop": float(actual.get_editor_property("depth_of_field_fstop")),
        "shutter_reciprocal_seconds": float(actual.get_editor_property("camera_shutter_speed")),
        "iso": float(actual.get_editor_property("camera_iso")),
        "bias": float(actual.get_editor_property("auto_exposure_bias")),
        "ev100_from_physical_settings": math.log2(actual.get_editor_property("depth_of_field_fstop") ** 2 * actual.get_editor_property("camera_shutter_speed") * 100 / actual.get_editor_property("camera_iso")),
        "theoretical_gain_relative_to_ev0_not_verified": 1.0 / 1024.0,
        "post_process_blend_weight": float(capture.get_editor_property("post_process_blend_weight")),
        "bloom": float(actual.get_editor_property("bloom_intensity")),
        "motion_blur": float(actual.get_editor_property("motion_blur_amount")),
    }
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    weapon = fixture_actor(actors, unreal.StaticMeshActor, "weapon")
    component = weapon.get_component_by_class(unreal.StaticMeshComponent)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
        raise RuntimeError("Fixture weapon component did not retain NoCollision")
    weapon.set_actor_scale3d(unreal.Vector(1, 1, 1))
    weapon.set_actor_rotation(unreal.Rotator(), False)
    weapon.set_actor_location(unreal.Vector(), False, True)
    for entry in MANIFEST["imports"]:
        STAGE = entry["piece"] + "_load"
        mesh = unreal.load_asset(entry["expected_mesh_package"])
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Actual weapon StaticMesh missing: " + entry["expected_mesh_package"])
        component.set_static_mesh(mesh)
        REFERENCES.append(mesh)
        textures = []
        for name in ("Color", "Normal", "Roughness", "Metallic"):
            texture = unreal.load_asset(entry["destination"] + "/" + name)
            if not isinstance(texture, unreal.Texture2D):
                raise RuntimeError("Imported texture missing: " + name)
            textures.append(texture)
        REFERENCES.extend(textures)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        yield after(0.5)
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        if component.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
            raise RuntimeError("Fixture component did not accept NoCollision after mesh loading")
        bounds = mesh.get_bounds()
        centre = bounds.origin
        radius = float(bounds.sphere_radius)
        if radius <= 0:
            raise RuntimeError("Actual mesh bounds are empty")
        width = 2.0 * radius * 1.15
        capture.set_editor_property("ortho_width", width)
        item = {"piece": entry["piece"], "mesh": mesh_info(mesh, subsystem),
                "actor_transform": {"location_world_cm": vector(weapon.get_actor_location()), "rotation": rotation(weapon.get_actor_rotation()), "scale": vector(weapon.get_actor_scale3d())},
                "component_collision": str(component.get_collision_enabled()),
                "actual_component_materials": [object_info(component.get_material(index)) for index in range(component.get_num_materials())],
                "texture_sizes_after_finish_loading": {texture.get_name(): [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()] for texture in textures},
                "views": []}
        REPORT["pieces"].append(item)
        description = mesh.get_static_mesh_description(0)
        if description is None:
            raise RuntimeError("LOD0 source MeshDescription missing")
        count = description.get_vertex_count()
        coordinates = []
        for index in range(count):
            vertex_id = unreal.VertexID(id_value=index)
            if not description.is_vertex_valid(vertex_id):
                raise RuntimeError("Imported MeshDescription vertex IDs are sparse; dense export is not complete")
            coordinates.append([index] + vector(description.get_vertex_position(vertex_id)))
            if (index + 1) % 2000 == 0:
                yield lambda: True
        points_path = OUT / (entry["piece"] + "-ue-local-lod0-vertices.json")
        points_path.write_text(json.dumps({"coordinate_space": "Actual imported UE mesh local centimetres; actor identity transform", "mesh": object_info(mesh), "vertex_count": count, "vertices_id_xyz": coordinates}, separators=(",", ":")), encoding="utf-8")
        item["source_vertices"] = {"path": str(points_path), "count": count, "id_range_dense_verified": True}
        for name, direction in (("positive-x", unreal.Vector(1, 0, 0)), ("positive-y", unreal.Vector(0, 1, 0)), ("positive-z", unreal.Vector(0, 0, 1)), ("oblique", unreal.Vector(1, -1, 0.7))):
            STAGE = entry["piece"] + "_" + name
            camera_location = centre + direction * (3.0 * radius)
            camera_rotation = unreal.MathLibrary.find_look_at_rotation(camera_location, centre)
            capture_actor.set_actor_location(camera_location, False, True)
            capture_actor.set_actor_rotation(camera_rotation, False)
            # A separate Slate tick settles the component and camera before capture.
            yield lambda: True
            capture.capture_scene()
            yield lambda: True
            filename = entry["piece"] + "-" + name + ".png"
            unreal.RenderingLibrary.export_render_target(world, target, str(OUT), filename)
            image_path = OUT / filename
            if not image_path.is_file():
                raise RuntimeError("SceneCapture export did not produce " + str(image_path))
            item["views"].append({"png": str(image_path), "view_from": name, "camera_location_world_cm": vector(capture_actor.get_actor_location()), "camera_rotation": rotation(capture_actor.get_actor_rotation()), "target_world_cm": vector(centre), "projection": str(capture.get_editor_property("projection_type")), "ortho_width_cm": float(capture.get_editor_property("ortho_width")), "render_target_format": "RTF_RGBA8", "pixels": [1024, 1024], "capture_source": str(capture.get_editor_property("capture_source")), "visual_review": "pending"})
    REPORT["capture_complete"] = True


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
ITERATOR = run()
PENDING = None
IN_TICK = False
DEADLINE = time.monotonic() + 240


def finish():
    REPORT["finished_utc"] = datetime.now(timezone.utc).isoformat()
    REPORT["stage"] = STAGE
    REPORT["quit_requested"] = True
    (OUT / "render-facts.json").write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.log("TASK070 weapon render evidence: " + str(OUT))
    unreal.SystemLibrary.quit_editor()


def tick(delta):
    global PENDING, IN_TICK
    if IN_TICK:
        return
    IN_TICK = True
    try:
        if time.monotonic() > DEADLINE:
            raise TimeoutError(STAGE)
        if PENDING is not None and not PENDING():
            return
        PENDING = next(ITERATOR)
    except StopIteration:
        finish()
    except Exception:
        REPORT["error"] = traceback.format_exc()
        finish()
    finally:
        IN_TICK = False


HANDLE = unreal.register_slate_post_tick_callback(tick)
