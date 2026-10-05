"""Root-run public UE stone axe/real Hero single-node pose evidence.

Run only in a fresh dedicated QA editor with a real RHI. Does not play Combat,
change/save assets, author sockets, or claim gameplay/Owner acceptance.
"""
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import time
import traceback

import unreal

HERE = Path(__file__).resolve().parent
GRIP_CANDIDATE = "-task055gripcandidate" in unreal.SystemLibrary.get_command_line().lower()
PALM_CANDIDATE = "-task055palmcandidate" in unreal.SystemLibrary.get_command_line().lower()
HANDLE_TOPOLOGY = "-task055handletopology" in unreal.SystemLibrary.get_command_line().lower()
LOCAL_SECTION_CANDIDATE = "-task055localsectioncandidate" in unreal.SystemLibrary.get_command_line().lower()
SHORT_CANDIDATE = GRIP_CANDIDATE or PALM_CANDIDATE or LOCAL_SECTION_CANDIDATE
if sum((GRIP_CANDIDATE, PALM_CANDIDATE, LOCAL_SECTION_CANDIDATE, HANDLE_TOPOLOGY)) > 1:
    raise RuntimeError("Stone axe evidence modes require separate opt-in runs")
OUT = Path(unreal.Paths.project_saved_dir()) / "Task055" / ("stone-axe-palm-candidate" if PALM_CANDIDATE else "stone-axe-grip-candidate" if GRIP_CANDIDATE else "stone-axe-single-node")
if HANDLE_TOPOLOGY:
    OUT = Path(unreal.Paths.project_saved_dir()) / "Task055/stone-axe-handle-topology"
if LOCAL_SECTION_CANDIDATE:
    OUT = Path(unreal.Paths.project_saved_dir()) / "Task055/stone-axe-local-section-candidate"
OUT.mkdir(parents=True, exist_ok=True)
HERO_PATH = "/Game/Characters/Hero/UE5/SK_Hero"
AXE_PATH = "/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe"
ATTACK_PATH = "/Game/Characters/Hero/AnimationV2/A_Hero_Attack"
CHOP_PATH = "/Game/Characters/Hero/AnimationV2/A_Hero_Chop"
BONES = ("hand_r", "hand_l", "head", "spine_03", "pelvis", "foot_r", "root")
CROP = (1.3333333655248714, 2.2500000543232206)
LIGHT_MOVE = {"windup_seconds": 0.35, "active_seconds": 0.18, "recovery_seconds": 0.47, "duration_seconds": 1.0}
REPORT = {"captured_utc": datetime.now(timezone.utc).isoformat(), "engine_version": unreal.SystemLibrary.get_engine_version(),
          "project_dir": unreal.Paths.project_dir(), "command_line": unreal.SystemLibrary.get_command_line(),
          "method": "Unsaved editor fixture, actual Hero constructor/held axe, SingleNode SetPosition(false), editor ticks, compressed AnimPose pose verification and SceneCapture next-tick PNG export",
          "sampling_complete": False, "clips": {}, "images": [], "stone_axe_light_move_source": "Source/Hearthward/Combat/HearthwardCombatRules.h blunt light", "move": LIGHT_MOVE,
          "old031_actual_source_window_seconds": list(CROP), "asset_mutations": [],
          "limitations": ["Single-node pose excludes native graph blend and normal input/action clock", "Height peak is not a contact event", "No damage/collision acceptance measured", "No sockets or grip alignment changed"]}
REPORT["mode"] = "temporary_grip_candidate_four_poses" if GRIP_CANDIDATE else "original_dense_pose_evidence"
if GRIP_CANDIDATE:
    REPORT["limitations"][-1] = "Unsaved fixture HeldAxe relative translation changed once to place candidate Grip on hand_r wrist pivot; finger/palm fit and normal equip acceptance remain unverified"
if PALM_CANDIDATE:
    REPORT["mode"] = "temporary_palm_two_candidates_source_0_55"
    REPORT["limitations"][-1] = "Unsaved HeldAxe translation/rotation changed for two measured-surface-derived candidates at scale .7; no palm/finger-fit acceptance or Content save"
if LOCAL_SECTION_CANDIDATE:
    REPORT["mode"] = "transient_local_section_candidate_source_0_55"
    REPORT["limitations"][-1] = "Transient mesh local-section geometry and unsaved Held binding candidate; source cached vertices restored, no Content save or production grip acceptance"
STAGE = "setup"
KEEP = []


def v(value):
    return [float(value.x), float(value.y), float(value.z)]


def q(value):
    return [float(value.x), float(value.y), float(value.z), float(value.w)]


def tf(value):
    return {"translation_cm": v(value.translation), "rotation_quat_xyzw": q(value.rotation), "scale_xyz": v(value.scale3d)}


def info(value):
    return {"path": value.get_path_name(), "class": value.get_class().get_name()} if value else None


def after(seconds):
    end = time.monotonic() + seconds
    return lambda: time.monotonic() >= end


def spawn(actors, cls, label, location=unreal.Vector(), rot=unreal.Rotator()):
    actor = actors.spawn_actor_from_class(cls, location, rot)
    if actor is None:
        raise RuntimeError("Could not spawn " + label)
    actor.set_actor_label("Task055_" + label)
    actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    KEEP.append(actor)
    return actor


def poses_match(mesh, expected, evidence):
    errors = {}
    for name in ("hand_r", "head", "pelvis"):
        actual = mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_COMPONENT)
        desired = expected[name]
        a, b = q(actual.rotation), q(desired.rotation)
        quaternion_error = min(math.sqrt(sum((a[i] - b[i]) ** 2 for i in range(4))), math.sqrt(sum((a[i] + b[i]) ** 2 for i in range(4))))
        errors[name] = {"translation_error_cm": (actual.translation - desired.translation).length(), "quaternion_error": quaternion_error, "scale_error": (actual.scale3d - desired.scale3d).length()}
    evidence["actual_single_node_position_seconds"] = mesh.get_position()
    evidence["last_pose_errors"] = errors
    return all(x["translation_error_cm"] <= 0.05 and x["quaternion_error"] <= 0.002 and x["scale_error"] <= 0.01 for x in errors.values())


def run():
    global STAGE
    if "-nullrhi" in unreal.SystemLibrary.get_command_line().lower():
        raise RuntimeError("Stone axe evidence requires a real RHI")
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        raise RuntimeError("No fresh editor world")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    REPORT["world"] = info(world)
    REPORT["lights"] = []
    for label, rot, intensity in (("key", unreal.Rotator(pitch=-35, yaw=-45, roll=0), 30.0), ("fill", unreal.Rotator(pitch=-20, yaw=135, roll=0), 10.0)):
        light = spawn(actors, unreal.DirectionalLight, label, rot=rot)
        light_component = light.get_component_by_class(unreal.DirectionalLightComponent)
        light_component.set_intensity(intensity)
        REPORT["lights"].append({"actor": info(light), "intensity_lux": float(light_component.get_editor_property("intensity")), "rotation": str(light.get_actor_rotation())})
    sky = spawn(actors, unreal.SkyLight, "sky").get_component_by_class(unreal.SkyLightComponent)
    cube = unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap")
    if not isinstance(cube, unreal.TextureCube):
        raise RuntimeError("Actual fixture sky cubemap missing")
    sky.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.set_cubemap(cube)
    sky.set_intensity(1.0)
    REPORT["sky_light"] = {"cubemap": info(sky.get_editor_property("cubemap")), "intensity": float(sky.get_editor_property("intensity")), "source_type": str(sky.get_editor_property("source_type"))}
    capture_actor = spawn(actors, unreal.SceneCapture2D, "capture")
    capture = capture_actor.capture_component2d
    target = unreal.RenderingLibrary.create_render_target2d(world, 1024, 1024, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    KEEP.append(target)
    capture.texture_target = target
    capture.capture_source = unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
    capture.set_editor_property("projection_type", unreal.CameraProjectionMode.ORTHOGRAPHIC)
    capture.set_editor_property("capture_every_frame", False)
    capture.set_editor_property("capture_on_movement", False)
    settings = capture.get_editor_property("post_process_settings")
    for name, value in (("override_auto_exposure_method", True), ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL), ("override_auto_exposure_apply_physical_camera_exposure", True), ("auto_exposure_apply_physical_camera_exposure", True), ("override_depth_of_field_fstop", True), ("depth_of_field_fstop", 4.0), ("override_camera_shutter_speed", True), ("camera_shutter_speed", 64.0), ("override_camera_iso", True), ("camera_iso", 100.0), ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.0), ("override_bloom_intensity", True), ("bloom_intensity", 0.0), ("override_motion_blur_amount", True), ("motion_blur_amount", 0.0)):
        settings.set_editor_property(name, value)
    capture.set_editor_property("post_process_settings", settings)
    actual = capture.get_editor_property("post_process_settings")
    REPORT["exposure"] = {"method": str(actual.get_editor_property("auto_exposure_method")), "physical": bool(actual.get_editor_property("auto_exposure_apply_physical_camera_exposure")), "aperture_fstop": float(actual.get_editor_property("depth_of_field_fstop")), "shutter_reciprocal_seconds": float(actual.get_editor_property("camera_shutter_speed")), "iso": float(actual.get_editor_property("camera_iso")), "bias": float(actual.get_editor_property("auto_exposure_bias")), "ev100": math.log2(actual.get_editor_property("depth_of_field_fstop") ** 2 * actual.get_editor_property("camera_shutter_speed") * 100 / actual.get_editor_property("camera_iso")), "post_process_blend_weight": float(capture.get_editor_property("post_process_blend_weight"))}

    # Read the actual imported mesh at identity before placing the Hero.
    axe_asset = unreal.load_asset(AXE_PATH)
    if not isinstance(axe_asset, unreal.StaticMesh):
        raise RuntimeError("Actual stone axe mesh missing")
    if HANDLE_TOPOLOGY:
        import sys
        sys.path.insert(0, str(HERE))
        from export_stone_axe_handle_topology import export_handle_topology
        REPORT["mode"] = "actual_handle_source_topology_read_only"
        REPORT["actual_handle_topology"] = yield from export_handle_topology(axe_asset, OUT)
        REPORT["sampling_complete"] = True
        return
    bounds = axe_asset.get_bounds()
    REPORT["axe_asset"] = {**info(axe_asset), "bounds_local_cm": {"origin": v(bounds.origin), "extent": v(bounds.box_extent), "sphere_radius": bounds.sphere_radius}, "lod0_triangles": axe_asset.get_num_triangles(0), "material_slots": [info(axe_asset.get_material(i)) for i in range(len(axe_asset.get_editor_property("static_materials")))]}
    local_section_candidate = None
    local_section_mesh = None
    if LOCAL_SECTION_CANDIDATE:
        import sys
        sys.path.insert(0, str(HERE))
        from build_stone_axe_local_section_transient import build_local_section_transient
        local_section_candidate = json.loads((HERE / "stone-axe-local-handle-section-candidate.json").read_text(encoding="utf-8-sig"))
        REPORT["measured_local_section_candidate"] = local_section_candidate
        REPORT["local_section_mesh"] = {}
        local_section_mesh = yield from build_local_section_transient(axe_asset, local_section_candidate, REPORT["local_section_mesh"])
        KEEP.append(local_section_mesh)
    if not SHORT_CANDIDATE:
        description = axe_asset.get_static_mesh_description(0)
        if description is None:
            raise RuntimeError("Actual stone axe LOD0 source vertices missing")
        points = []
        for index in range(description.get_vertex_count()):
            vertex_id = unreal.VertexID(id_value=index)
            if not description.is_vertex_valid(vertex_id):
                raise RuntimeError("Axe source vertex IDs are sparse; dense export is incomplete")
            points.append([index] + v(description.get_vertex_position(vertex_id)))
            if (index + 1) % 2000 == 0:
                yield lambda: True
        vertex_path = OUT / "stone-axe-ue-local-vertices.json"
        vertex_path.write_text(json.dumps({"mesh": info(axe_asset), "coordinate_space": "UE mesh local centimetres", "vertices_id_xyz": points}, separators=(",", ":")), encoding="utf-8")
        REPORT["axe_asset"]["actual_local_vertices"] = str(vertex_path)
    endpoints_file = HERE / "stone-axe-measured-endpoints.json"
    endpoints = {}
    if endpoints_file.is_file():
        data = json.loads(endpoints_file.read_text(encoding="utf-8-sig"))
        if data["mesh_path"] != AXE_PATH or data["coordinate_space"] != "UE mesh local cm" or not data["selection_evidence"]:
            raise RuntimeError("Endpoint candidate file must identify actual mesh local selection evidence")
        endpoints = {name: unreal.Vector(*data["candidates"][name]) for name in ("Grip", "BladeBase", "BladeTip")}
        REPORT["measured_endpoint_candidates"] = data
    else:
        if SHORT_CANDIDATE:
            raise RuntimeError("Short candidate modes require measured mesh-local endpoint JSON")
        REPORT["measured_endpoint_candidates"] = {"status": "Not yet selected from the actual imported mesh; exported vertices and actual axe transforms permit later exact world-coordinate evaluation without replay", "file_to_supply_after_selection": str(endpoints_file)}
    if not SHORT_CANDIDATE:
        inspection = spawn(actors, unreal.StaticMeshActor, "axe_local")
        inspection_component = inspection.get_component_by_class(unreal.StaticMeshComponent)
        inspection_component.set_static_mesh(axe_asset)
        inspection_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        yield after(0.5)
        for name, direction in (("x", unreal.Vector(1, 0, 0)), ("y", unreal.Vector(0, 1, 0)), ("z", unreal.Vector(0, 0, 1))):
            centre = bounds.origin
            yield from photograph(world, capture_actor, capture, target, "axe-local-" + name, centre + direction * (3 * bounds.sphere_radius), centre, 2.3 * bounds.sphere_radius)
        actors.destroy_actor(inspection)

    hero = spawn(actors, unreal.HearthwardCharacter, "hero", unreal.Vector(0, 0, 90))
    mesh = hero.get_component_by_class(unreal.SkeletalMeshComponent)
    if mesh.get_skinned_asset().get_path_name().split(".")[0] != HERO_PATH:
        raise RuntimeError("Fixture is not the actual production Hero mesh")
    held = next(c for c in hero.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == "HeldAxe")
    if held.get_editor_property("static_mesh") != axe_asset or held.get_attach_parent() != mesh or str(held.get_attach_socket_name()) != "hand_r" or held.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
        raise RuntimeError("Actual constructor stone axe attachment is different")
    REPORT["constructor"] = {"hero": info(hero), "mesh_world": tf(mesh.get_world_transform()), "mesh_relative": tf(mesh.get_relative_transform()), "held_relative": tf(held.get_relative_transform()), "held_absolute_scale": bool(held.get_editor_property("absolute_scale")), "held_collision": str(held.get_collision_enabled()), "held_was_visible": bool(held.get_editor_property("visible")), "attach_socket": str(held.get_attach_socket_name())}
    finger_bones = []
    if SHORT_CANDIDATE:
        names = [str(mesh.get_bone_name(index)) for index in range(mesh.get_num_bones())]
        finger_bones = [name for name in names if name.endswith("_r") and any(part in name.lower() for part in ("thumb", "index", "middle", "ring", "pinky"))]
        REPORT["actual_bone_names"] = names
        REPORT["available_right_finger_bones"] = finger_bones
        REPORT["grip_reference"] = {"bone": "hand_r", "meaning": "Actual wrist bone pivot, not a measured palm centre", "finger_selection": "Existing runtime bone names ending _r and containing thumb/index/middle/ring/pinky"}
    palm_candidate = None
    if PALM_CANDIDATE:
        palm_candidate = json.loads((HERE / "stone-axe-palm-surface-candidate.json").read_text(encoding="utf-8-sig"))
        if "requires actual root close-up" not in palm_candidate["status"] or palm_candidate["source_time_seconds"] < .54:
            raise RuntimeError("Palm candidate must identify actual source .55 geometry evidence")
        REPORT["measured_palm_candidate"] = palm_candidate
        REPORT["grip_reference"] = {"bone": "hand_r", "meaning": "Wrist bone transform is the coordinate frame; candidate target uses measured palm/distal skin surfaces, not wrist pivot", "target_hand_local": palm_candidate["grip_target"]["hand_r_local"]}
    if LOCAL_SECTION_CANDIDATE:
        endpoints = {name: axe_asset.find_socket(name).get_editor_property("relative_location") for name in ("Grip", "BladeBase", "BladeTip")}
        held.set_static_mesh(local_section_mesh)
        held.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        REPORT["grip_reference"] = {"bone": "hand_r", "meaning": "Actual hand frame and actual authored Grip; candidate target is measured geometry, no palm-fit acceptance", "target_hand_local": local_section_candidate["temporary_grip_target_hand_r_local"]}
    held.set_visibility(True)
    REPORT["fixture_visibility_change"] = "HeldAxe made visible only for sampling; no normal equip/Combat transaction executed"
    mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    mesh.set_update_animation_in_editor(True)
    mesh.set_editor_property("visibility_based_anim_tick_option", unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
    mesh.set_editor_property("enable_update_rate_optimizations", False)
    mesh.set_editor_property("suppress_notify_event_dispatch", True)
    options = unreal.AnimPoseEvaluationOptions()
    options.set_editor_property("evaluation_type", unreal.AnimDataEvalType.COMPRESSED)
    options.set_editor_property("optional_skeletal_mesh", mesh.get_skinned_asset())
    options.set_editor_property("should_retarget", True)
    options.set_editor_property("extract_root_motion", False)
    clips = (("current_attack", ATTACK_PATH, 60),) if SHORT_CANDIDATE else (("current_attack", ATTACK_PATH, 60), ("original_chop", CHOP_PATH, 24))
    for label, path, fps in clips:
        STAGE = label
        clip = unreal.load_asset(path)
        if not isinstance(clip, unreal.AnimSequence) or clip.get_editor_property("skeleton") != mesh.get_skinned_asset().get_editor_property("skeleton"):
            raise RuntimeError("Actual Hero clip missing or skeleton differs: " + path)
        length = clip.get_play_length()
        mesh.play_animation(clip, False)
        mesh.set_play_rate(0.0)
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        if SHORT_CANDIDATE:
            render_times = {0.6} if PALM_CANDIDATE or LOCAL_SECTION_CANDIDATE else {0.0, 0.6, 0.8, 1.0}
            times = {length * normalized_time for normalized_time in render_times}
            if PALM_CANDIDATE or LOCAL_SECTION_CANDIDATE:
                times.add(0.0)
            REPORT["candidate_phase_source_boundaries_seconds"] = {"windup_start": 0.0, "windup_to_active": length * 0.6, "active_to_recovery": length * 0.8, "clip_end": length}
        elif label == "current_attack":
            times = {length * i / 60 for i in range(61)}
            render_times = {0.0, 0.35, 0.53, 6.0 / 11.0, 0.70, 1.0}
            times.update(length * action_time for action_time in render_times)
        else:
            count = round(length * fps)
            times = {length * i / count for i in range(count + 1)}
            render_times = {CROP[0], CROP[0] + 0.5, CROP[1]}
            times.update(render_times)
        notifies = unreal.AnimationLibrary.get_animation_notify_events(clip)
        item = {"asset": info(clip), "actual_length_seconds": length, "single_node_play_rate": 0.0, "sampling_method": "set_position then editor ticks until actual component hand/head/pelvis match separately evaluated compressed asset pose", "notify_events": [{"trigger_time": unreal.AnimationLibrary.get_anim_notify_event_trigger_time(event), "duration": unreal.AnimationLibrary.get_anim_notify_event_duration(event)} for event in notifies], "samples": []}
        REPORT["clips"][label] = item
        for sample_time in sorted(times):
            STAGE = label + ":" + str(round(sample_time, 5))
            pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(clip, sample_time, options)
            if not unreal.AnimPoseExtensions.is_valid(pose):
                raise RuntimeError("Public compressed AnimPose evaluation is invalid")
            expected = {name: unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD) for name in BONES}
            mesh.set_position(sample_time, False)
            sample = {"clip_time_seconds": sample_time, "requested_compressed_component_pose": {name: tf(expected[name]) for name in BONES}}
            item["samples"].append(sample)
            pose_deadline = time.monotonic() + 3.0
            def ready():
                if poses_match(mesh, expected, sample):
                    return True
                if time.monotonic() >= pose_deadline:
                    raise TimeoutError("Editor pose did not match actual requested compressed clip time: " + STAGE)
                return False
            yield ready
            yield lambda: True
            if not poses_match(mesh, expected, sample) or abs(mesh.get_position() - sample_time) > 0.0001:
                raise RuntimeError("Actual sampled pose or single-node position drifted")
            if GRIP_CANDIDATE and "temporary_grip_translation" not in REPORT:
                before = held.get_world_transform()
                before_grip = unreal.MathLibrary.transform_location(before, endpoints["Grip"])
                wrist = mesh.get_socket_transform("hand_r", unreal.RelativeTransformSpace.RTS_WORLD)
                REPORT["temporary_grip_translation"] = {"method": "One SetWorldLocation translation after actual source-zero pose; attachment computes relative translation through actual socket transform including its bone scale", "before_world": tf(before), "before_relative": tf(held.get_relative_transform()), "actual_hand_socket_world": tf(wrist), "before_grip_world_cm": v(before_grip), "before_grip_to_wrist_distance_cm": (before_grip - wrist.translation).length(), "requested_world_offset_cm": v(wrist.translation - before_grip)}
                held.set_world_location(before.translation + wrist.translation - before_grip, False, True)
                yield lambda: True
                if not poses_match(mesh, expected, sample) or abs(mesh.get_position() - sample_time) > 0.0001:
                    raise RuntimeError("Actual source-zero pose drifted during temporary grip translation")
                if held.get_attach_parent() != mesh or str(held.get_attach_socket_name()) != "hand_r":
                    raise RuntimeError("Temporary grip translation changed actual attachment")
                actual_transform = held.get_world_transform()
                after_grip = unreal.MathLibrary.transform_location(actual_transform, endpoints["Grip"])
                REPORT["temporary_grip_translation"].update({"after_world": tf(actual_transform), "after_relative": tf(held.get_relative_transform()), "after_grip_world_cm": v(after_grip), "after_grip_to_wrist_distance_cm": (after_grip - mesh.get_socket_location("hand_r")).length(), "actual_attach_socket": str(held.get_attach_socket_name()), "actual_absolute_scale": bool(held.get_editor_property("absolute_scale"))})
            sample["actual_clip_position_seconds"] = mesh.get_position()
            sample["actual_component_pose"] = {name: tf(mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_COMPONENT)) for name in BONES}
            sample["actual_world_pose"] = {name: tf(mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)) for name in BONES}
            axe_transform = held.get_world_transform()
            sample["actual_held_axe_world"] = tf(axe_transform)
            sample["actual_held_axe_relative"] = tf(held.get_relative_transform())
            sample["actual_mesh_world"] = tf(mesh.get_world_transform())
            sample["endpoint_world_candidates_cm"] = {name: v(unreal.MathLibrary.transform_location(axe_transform, local)) for name, local in endpoints.items()}
            sample["axe_local_origin_to_hand_world_distance_cm"] = (axe_transform.translation - mesh.get_socket_location("hand_r")).length()
            if GRIP_CANDIDATE:
                wrist = mesh.get_socket_transform("hand_r", unreal.RelativeTransformSpace.RTS_WORLD)
                grip = unreal.MathLibrary.transform_location(axe_transform, endpoints["Grip"])
                sample["candidate_phase_source_boundary"] = next(name for name, seconds in REPORT["candidate_phase_source_boundaries_seconds"].items() if abs(seconds - sample_time) < 0.00001)
                sample["actual_hand_socket_world"] = tf(wrist)
                sample["grip_world_to_wrist_pivot_distance_cm"] = (grip - wrist.translation).length()
                sample["actual_right_finger_world_reference"] = {name: tf(mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)) for name in finger_bones}
                if sample["grip_world_to_wrist_pivot_distance_cm"] > 0.05:
                    raise RuntimeError("One-time temporary grip translation did not remain on actual wrist pivot")
            if label == "current_attack":
                action_time = sample_time / length
                sample["current_uniform_light_action_time_seconds"] = action_time
                sample["current_combat_phase"] = "windup" if action_time < 0.35 else "active" if action_time <= 0.53 else "recovery"
                sample["old031_source_time_projection_seconds"] = CROP[0] + (CROP[1] - CROP[0]) * action_time
                render = any(abs(action_time - t) < 0.00001 for t in render_times)
            else:
                render = any(abs(sample_time - t) < 0.00001 for t in render_times)
            if render and LOCAL_SECTION_CANDIDATE:
                if abs(sample_time-local_section_candidate["source_pose_seconds"]) > .0001:
                    raise RuntimeError("Local-section candidate is restricted to the actually measured source .55")
                hand_transform = mesh.get_socket_transform("hand_r", unreal.RelativeTransformSpace.RTS_WORLD)
                candidate_local = unreal.Vector(*local_section_candidate["temporary_grip_target_hand_r_local"])
                desired_grip = unreal.MathLibrary.transform_location(hand_transform, candidate_local)
                quaternion = local_section_candidate["relative_rotation_quat_xyzw"]
                held.set_world_scale3d(unreal.Vector(.7, .7, .7))
                held.set_relative_rotation(unreal.Quat(*quaternion).rotator(), False, True)
                rotated_world = held.get_world_transform()
                rotated_grip = unreal.MathLibrary.transform_location(rotated_world, endpoints["Grip"])
                held.set_world_location(rotated_world.translation + desired_grip - rotated_grip, False, True)
                yield lambda: True
                if not poses_match(mesh, expected, sample) or abs(mesh.get_position()-sample_time) > .0001:
                    raise RuntimeError("Actual .55 pose drifted during local-section candidate")
                if held.get_attach_parent() != mesh or str(held.get_attach_socket_name()) != "hand_r" or not held.get_editor_property("absolute_scale") or held.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION:
                    raise RuntimeError("Local-section candidate changed attachment/absolute-scale/collision contract")
                actual_world = held.get_world_transform()
                actual_hand = mesh.get_socket_transform("hand_r", unreal.RelativeTransformSpace.RTS_WORLD)
                actual_target = unreal.MathLibrary.transform_location(actual_hand, candidate_local)
                actual_grip = unreal.MathLibrary.transform_location(actual_world, endpoints["Grip"])
                error = (actual_grip-actual_target).length()
                if error > .05 or (actual_world.scale3d-unreal.Vector(.7, .7, .7)).length() > .0001:
                    raise RuntimeError("Local-section alignment or actual world scale differs")
                sample["temporary_local_section_candidate"] = {"target_hand_r_local": v(candidate_local), "actual_hand_socket_world": tf(actual_hand), "actual_grip_target_world_cm": v(actual_target), "actual_grip_world_cm": v(actual_grip), "grip_to_target_distance_cm": error, "actual_held_axe_world": tf(actual_world), "actual_held_axe_relative": tf(held.get_relative_transform()), "transient_mesh": info(held.get_editor_property("static_mesh")), "actual_collision": str(held.get_collision_enabled()), "actual_right_finger_world_reference": {name: tf(mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)) for name in finger_bones}, "actual_endpoint_world_cm": {name: v(unreal.MathLibrary.transform_location(actual_world, local)) for name, local in endpoints.items()}}
                suffix = label + "-" + f"{sample_time:.6f}".replace(".", "_") + "-local-section"
                centre = unreal.Vector(0, 0, 100)
                yield from photograph(world, capture_actor, capture, target, suffix+"-full", centre+unreal.Vector(350, -400, 160), centre, 400.)
                for camera_name, rigid_offset in (("palm-reverse", [-25, 0, -100]), ("index-side", [100, 0, 0])):
                    eye = actual_target + unreal.MathLibrary.transform_direction(actual_hand, unreal.Vector(*rigid_offset))
                    yield from photograph(world, capture_actor, capture, target, suffix+"-"+camera_name, eye, actual_target, 30.)
            elif render and PALM_CANDIDATE:
                hand_transform = mesh.get_socket_transform("hand_r", unreal.RelativeTransformSpace.RTS_WORLD)
                candidate_local = unreal.Vector(*palm_candidate["grip_target"]["hand_r_local"])
                desired_grip = unreal.MathLibrary.transform_location(hand_transform, candidate_local)
                original_quaternion = q(held.get_relative_transform().rotation)
                candidates = (("translation-only", original_quaternion), ("minimum-align", palm_candidate["temporary_attachment_candidate"]["relative_rotation_quat_xyzw"]))
                sample["temporary_palm_candidates"] = []
                for candidate_name, quaternion in candidates:
                    before_world = held.get_world_transform()
                    held.set_relative_rotation(unreal.Quat(*quaternion).rotator(), False, True)
                    rotated_world = held.get_world_transform()
                    rotated_grip = unreal.MathLibrary.transform_location(rotated_world, endpoints["Grip"])
                    held.set_world_location(rotated_world.translation + desired_grip - rotated_grip, False, True)
                    yield lambda: True
                    if not poses_match(mesh, expected, sample) or abs(mesh.get_position() - sample_time) > 0.0001:
                        raise RuntimeError("Actual .55 pose drifted during temporary palm candidate")
                    if held.get_attach_parent() != mesh or str(held.get_attach_socket_name()) != "hand_r" or not held.get_editor_property("absolute_scale"):
                        raise RuntimeError("Palm candidate changed production attachment/absolute-scale contract")
                    actual_world = held.get_world_transform()
                    actual_hand = mesh.get_socket_transform("hand_r", unreal.RelativeTransformSpace.RTS_WORLD)
                    actual_target = unreal.MathLibrary.transform_location(actual_hand, candidate_local)
                    actual_grip = unreal.MathLibrary.transform_location(actual_world, endpoints["Grip"])
                    error = (actual_grip - actual_target).length()
                    if error > .05 or (actual_world.scale3d - unreal.Vector(.7, .7, .7)).length() > .0001:
                        raise RuntimeError("Palm candidate alignment or actual .7 world scale differs")
                    evidence = {"name": candidate_name, "meaning": "Temporary measured-surface-derived target, requires rendered palm/finger-fit review", "target_hand_r_local": v(candidate_local), "actual_hand_socket_world": tf(actual_hand), "actual_grip_target_world_cm": v(actual_target), "actual_grip_world_cm": v(actual_grip), "grip_to_target_distance_cm": error, "requested_relative_quaternion_xyzw": quaternion, "before_world": tf(before_world), "actual_held_axe_world": tf(actual_world), "actual_held_axe_relative": tf(held.get_relative_transform()), "actual_right_finger_world_reference": {name: tf(mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)) for name in finger_bones}, "actual_endpoint_world_cm": {name: v(unreal.MathLibrary.transform_location(actual_world, local)) for name, local in endpoints.items()}}
                    sample["temporary_palm_candidates"].append(evidence)
                    suffix = label + "-" + f"{sample_time:.6f}".replace(".", "_") + "-palm-" + candidate_name
                    centre = unreal.Vector(0, 0, 100)
                    yield from photograph(world, capture_actor, capture, target, suffix + "-full", centre + unreal.Vector(350, -400, 160), centre, 400.0)
                    eye = actual_target + unreal.MathLibrary.transform_direction(actual_hand, unreal.Vector(0, 0, 1)) * 100.0 + unreal.MathLibrary.transform_direction(actual_hand, unreal.Vector(1, 0, 0)) * 25.0
                    yield from photograph(world, capture_actor, capture, target, suffix + "-grip", eye, actual_target, 70.0)
            elif render:
                suffix = label + "-" + f"{sample_time:.6f}".replace(".", "_")
                centre = unreal.Vector(0, 0, 100)
                yield from photograph(world, capture_actor, capture, target, suffix + "-full", centre + unreal.Vector(350, -400, 160), centre, 400.0)
                hand = mesh.get_socket_location("hand_r")
                yield from photograph(world, capture_actor, capture, target, suffix + "-grip", hand + unreal.Vector(180, -220, 60), hand, 160.0)
    if not SHORT_CANDIDATE:
        attack = REPORT["clips"]["current_attack"]["samples"]
        chop = REPORT["clips"]["original_chop"]["samples"]
        for label, samples, eligible in (("attack", attack, attack), ("chop_first_half", chop, [x for x in chop if 0 < x["clip_time_seconds"] < REPORT["clips"]["original_chop"]["actual_length_seconds"] / 2])):
            peak = max(eligible, key=lambda x: x["actual_world_pose"]["hand_r"]["translation_cm"][2])
            REPORT[label + "_hand_height_peak"] = {"clip_time_seconds": peak["clip_time_seconds"], "hand_world_z_cm": peak["actual_world_pose"]["hand_r"]["translation_cm"][2], "current_uniform_light_action_time_seconds": peak.get("current_uniform_light_action_time_seconds"), "meaning": "Measured hand height maximum within sampled range; not blade contact or damage time"}
    REPORT["sampling_complete"] = True


def photograph(world, camera, capture, target, name, eye, centre, width):
    camera.set_actor_location(eye, False, True)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye, centre), False)
    capture.set_editor_property("ortho_width", width)
    yield lambda: True
    capture.capture_scene()
    yield lambda: True
    filename = name + ".png"
    unreal.RenderingLibrary.export_render_target(world, target, str(OUT), filename)
    if not (OUT / filename).is_file():
        raise RuntimeError("No actual PNG export: " + filename)
    REPORT["images"].append({"file": str(OUT / filename), "camera_world": tf(camera.get_actor_transform()), "target_world_cm": v(centre), "ortho_width_cm": width, "pixels": [1024, 1024], "review": "pending"})


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
FLOW = run()
PENDING = None
IN_TICK = False
DEADLINE = time.monotonic() + 240


def finish():
    REPORT["finished_utc"] = datetime.now(timezone.utc).isoformat()
    REPORT["stage"] = STAGE
    REPORT["quit_requested"] = True
    (OUT / "pose-and-axe-facts.json").write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(HANDLE)
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
        PENDING = next(FLOW)
    except StopIteration:
        finish()
    except Exception:
        REPORT["error"] = traceback.format_exc()
        finish()
    finally:
        IN_TICK = False


HANDLE = unreal.register_slate_post_tick_callback(tick)
