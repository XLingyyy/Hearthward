"""Reopen TASK-026, validate serialized assets, and run a real PIE smoke pass."""

import json
import time
import traceback
from pathlib import Path

import unreal


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
out = root / "Saved" / "Task026"
out.mkdir(parents=True, exist_ok=True)
report_path = out / "verification.json"
map_path = "/Game/Hearthward/World/Natural/L_NaturalWorld"
asset_root = "/Game/Hearthward/Assets/NaturalWorld"
world_root = "/Game/Hearthward/World/Natural"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report = {
    "passed": False,
    "map": map_path,
    "checks": {},
    "actor_instances": {},
    "positions": {},
    "screenshots": [],
    "input_method": "Enhanced Input action injection in real PIE; long route timed separately",
}
state = {"input": (0.0, 0.0)}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=45):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    return wait(lambda: time.monotonic() >= state["delay_until"], seconds + 5)


def set_delay(seconds):
    state["delay_until"] = time.monotonic() + seconds
    return delay(seconds)


def place(name, x, y, z, yaw):
    pawn = state["pawn"]
    state["input"] = (0.0, 0.0)
    pawn.character_movement.stop_movement_immediately()
    pawn.set_actor_location(unreal.Vector(x, y, z), False, True)
    state["pc"].set_control_rotation(unreal.Rotator(pitch=-8, yaw=yaw, roll=0))
    report["positions"][name] = [x, y, z]


def capture(name):
    path = out / (name + ".png")
    state["pawn"].set_actor_hidden_in_game(True)
    unreal.SystemLibrary.execute_console_command(
        state["world"], f'HighResShot 1280x720 filename="{path.as_posix()}"', state["pc"]
    )
    report["screenshots"].append(path.name)


def run():
    reserved = [
        map_path,
        asset_root + "/Textures/T_GrassGround_D",
        asset_root + "/Textures/T_Dirt_D",
        asset_root + "/Textures/T_Rock_D",
        asset_root + "/Materials/M_GroundGrass",
        asset_root + "/Materials/M_Soil",
        asset_root + "/Materials/M_Rock",
        asset_root + "/Materials/M_Water",
        asset_root + "/Materials/M_Foliage",
        asset_root + "/Materials/M_Bark",
        asset_root + "/Materials/M_Trail",
        world_root + "/BP_NaturalWorldGameMode",
    ]
    for path in reserved:
        check("asset_exists_" + path.rsplit("/", 1)[-1], unreal.EditorAssetLibrary.does_asset_exist(path))

    check("reopen_map", levels.load_level(map_path))
    all_actors = editor_actors.get_all_level_actors()
    proxies = [a for a in all_actors if a.get_class().get_name() == "LandscapeStreamingProxy"]
    report["loaded_editor_landscape_proxies"] = len(proxies)
    check("world_partition_region_loaded", 0 < len(proxies) <= 64)
    check(
        "landscape_xy_scale_200cm",
        all(abs(a.get_actor_scale3d().x - 200.0) < 0.1 and abs(a.get_actor_scale3d().y - 200.0) < 0.1 for a in proxies),
    )
    external_root = root / "Content/__ExternalActors__/Hearthward/World/Natural"
    check("world_partition_external_actor_dir", external_root.is_dir())
    external_packages = list(external_root.rglob("*.uasset"))
    report["external_actor_package_count"] = len(external_packages)
    check("world_partition_external_actor_packages", len(external_packages) >= 170)

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True, include_hard_package_references=True
    )
    dependencies = [str(item) for item in registry.get_dependencies(map_path, options)]
    project_dependencies = [item for item in dependencies if item.startswith("/Game/")]
    report["project_dependencies"] = project_dependencies
    check(
        "task_owned_project_dependencies",
        all(
            item == map_path
            or item.startswith(map_path + "_")
            or item.startswith(world_root + "/")
            or item.startswith(asset_root + "/")
            or item.startswith("/Game/__ExternalActors__/Hearthward/World/Natural/")
            or item.startswith("/Game/__ExternalObjects__/Hearthward/World/Natural/")
            for item in project_dependencies
        ),
    )

    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor, 60)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world, 0)), 60)
    state["world"] = world
    state["pawn"] = unreal.GameplayStatics.get_player_pawn(world, 0)
    state["pc"] = unreal.GameplayStatics.get_player_controller(world, 0)
    yield set_delay(2.0)
    pawn = state["pawn"]
    pc = state["pc"]
    actions = sorted(
        [
            action
            for action in unreal.ObjectIterator(unreal.InputAction)
            if action.get_outer() == pawn and action.value_type == unreal.InputActionValueType.AXIS2D
        ],
        key=lambda action: action.get_name(),
    )
    check("existing_move_and_look_actions", len(actions) == 2)
    subsystems = [
        subsystem
        for subsystem in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
        if isinstance(subsystem.get_outer(), unreal.LocalPlayer)
        and subsystem.query_keys_mapped_to_action(actions[0])
    ]
    check("one_player_input", len(subsystems) == 1)
    state["input_subsystem"] = subsystems[0]
    state["move_action"] = actions[0]

    storage = next(s for s in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if s.get_outer() == world)
    local_ai = next(s for s in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if s.get_outer() == world)
    check("isolated_empty_storage", storage.get_item_count("wood") == 0)
    check("isolated_model_not_started", local_ai.get_server_process_id() == 0)
    check("no_saved_companion_fixture", not unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY"))
    loaded_natural = unreal.GameplayStatics.get_all_actors_with_tag(world, "TASK026.NATURAL_WORLD")
    report["loaded_natural_actor_count_at_spawn"] = len(loaded_natural)
    check("natural_batches_stream_at_spawn", len(loaded_natural) >= 3)
    for actor in loaded_natural:
        component = actor.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
        if component:
            report["actor_instances"][actor.get_actor_label()] = component.get_instance_count()

    place("meadow_start", -62500, -62500, 6200, 45)
    yield set_delay(2.0)
    start = pawn.get_actor_location()
    report["positions"]["meadow_settled"] = [start.x, start.y, start.z]
    check("player_settles_on_natural_base", 5000 < start.z < 5300)
    state["input"] = (0.0, 1.0)
    yield set_delay(5.0)
    state["input"] = (0.0, 0.0)
    moved = pawn.get_actor_location()
    report["positions"]["meadow_after_walk"] = [moved.x, moved.y, moved.z]
    check("normal_character_walk", (moved - start).length() > 1000 and 5000 < moved.z < 5300)
    capture("meadow-route")
    yield set_delay(1.0)

    place("west_ford", -11815, -50000, 6200, 0)
    yield set_delay(2.0)
    ford_start = pawn.get_actor_location()
    report["positions"]["ford_settled"] = [ford_start.x, ford_start.y, ford_start.z]
    state["input"] = (0.0, 1.0)
    yield set_delay(5.0)
    state["input"] = (0.0, 0.0)
    ford_end = pawn.get_actor_location()
    report["positions"]["ford_after_walk"] = [ford_end.x, ford_end.y, ford_end.z]
    check("ford_walkable", (ford_end - ford_start).length() > 1000 and 5000 < ford_end.z < 5400)
    capture("river-ford")
    yield set_delay(1.0)

    place("forest_observation", -105000, 85000, 6200, 35)
    yield set_delay(2.0)
    capture("forest-sentinel")
    yield set_delay(1.0)
    place("hill_observation", 90000, -45000, 6200, 25)
    yield set_delay(2.0)
    capture("hills-twin-fangs")
    yield set_delay(1.0)
    place("lake_observation", -10000, 90000, 9000, 30)
    yield set_delay(2.0)
    capture("river-crown")
    yield set_delay(1.0)

    state.pop("input_subsystem", None)
    state.pop("move_action", None)
    levels.editor_request_end_play()
    yield wait(lambda: not levels.is_in_play_in_editor(), 60)
    report["passed"] = all(report["checks"].values())


def finish(error=None):
    if error:
        report["error"] = error
    report["passed"] = not error and bool(report["checks"]) and all(report["checks"].values())
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    unreal.unregister_slate_post_tick_callback(handle)


flow = run()
pending = None
deadline = time.monotonic() + 240


def tick(_delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError("TASK-026 PIE smoke verification timed out")
        if state.get("input_subsystem"):
            x, y = state["input"]
            state["input_subsystem"].inject_input_vector_for_action(
                state["move_action"], unreal.Vector(x, y, 0), [], []
            )
        if pending:
            predicate, stage_deadline = pending
            if not predicate():
                if time.monotonic() > stage_deadline:
                    raise TimeoutError("TASK-026 stage timed out")
                return
        pending = next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
