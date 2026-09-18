"""Inspect serialized dependencies and operate the real pawn in two isolated PIE sessions."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task014"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
map_path = "/Game/Hearthward/Tests/Graybox/L_GrayboxValidation"
art = "/Game/Hearthward/Art/Graybox"
report = {"ok": False, "map": map_path, "prototype_only": True, "checks": {}, "positions": {}, "dependencies": {}, "input_method": "Enhanced Input action injection; physical keys separately recorded"}
state = {"input": (0, 0)}


def check(name, condition):
    report["checks"][name] = bool(condition)
    if not condition:
        raise AssertionError(name)


def wait(predicate, timeout=30):
    return predicate, time.monotonic() + timeout


def delay(seconds):
    until = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= until)


def position(name):
    v = state["pawn"].get_actor_location()
    report["positions"][name] = [v.x, v.y, v.z]
    return v


def place(x, y, z=95):
    state["input"] = (0, 0)
    pawn = state["pawn"]
    pawn.character_movement.stop_movement_immediately()
    pawn.set_actor_location(unreal.Vector(x, y, z), False, True)
    state["pc"].set_control_rotation(unreal.Rotator(pitch=-12, yaw=0, roll=0))


def capture(name):
    unreal.SystemLibrary.execute_console_command(state["world"], 'HighResShot 1280x720 filename="' + (out / (name + ".png")).as_posix() + '"', state["pc"])


def run():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    queue = [map_path] + [art + "/" + name for name in ["SM_Block100", "M_Ground", "M_Wall", "M_Accent", "M_Route"]]
    visited = set()
    while queue:
        package = queue.pop()
        if package in visited:
            continue
        visited.add(package)
        check("asset_exists_" + package, unreal.EditorAssetLibrary.does_asset_exist(package))
        deps = [str(d) for d in registry.get_dependencies(package, options)]
        report["dependencies"][package] = deps
        for dep in deps:
            if dep.startswith("/Game/"):
                check("isolated_reference_" + dep, dep == map_path or dep.startswith(art + "/"))
                queue.append(dep)
    check("six_project_packages", len(visited) == 6)
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world, 0)))
    yield delay(1)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    actions = sorted([a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer() == pawn and a.value_type == unreal.InputActionValueType.AXIS2D], key=lambda a: a.get_name())
    check("existing_move_and_look_actions", len(actions) == 2)
    subs = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(s.get_outer(), unreal.LocalPlayer) and s.query_keys_mapped_to_action(actions[0])]
    check("one_player_input", len(subs) == 1)
    state.update(world=world, pawn=pawn, pc=pc, sub=subs[0], move=actions[0])
    start = position("spawn")
    check("safe_player_start", abs(start.x) < 5 and abs(start.y) < 5 and 80 < start.z < 105)
    storage = next(s for s in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if s.get_outer() == world)
    ai = next(s for s in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if s.get_outer() == world)
    check("no_model_autostart_or_stock_grant", ai.get_server_process_id() == 0 and storage.get_item_count("wood") == 0)
    check("no_companion_fixture_saved", not unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY"))
    place(0, 0)
    state["input"] = (0, 1)
    yield delay(2)
    state["input"] = (0, 0)
    yield delay(.2)
    v = position("flat_walk")
    check("walk_along_distance_lane", v.x > 500 and abs(v.y) < 10 and 80 < v.z < 105)
    capture("lane")
    yield delay(.5)
    place(3200, 0)
    state["input"] = (0, 1)
    yield delay(2)
    state["input"] = (0, 0)
    v = position("collision_stop")
    check("wall_stops_capsule", 3500 < v.x < 3530 and pawn.get_velocity().length() < 5)
    place(700, -1100)
    yield delay(.3)
    state["input"] = (0, 1)
    yield delay(3)
    state["input"] = (0, 0)
    yield delay(.3)
    v = position("raised_platform")
    check("ramp_reaches_platform", 1650 < v.x < 2100 and 280 < v.z < 305)
    capture("ramp")
    yield delay(.5)
    place(2800, -1300)
    state["input"] = (0, 1)
    yield delay(2)
    state["input"] = (0, 0)
    v = position("gate_pass")
    check("clear_gate_is_walkable", v.x > 3350 and 80 < v.z < 105)
    place(2800, -1050)
    state["input"] = (0, 1)
    yield delay(2)
    state["input"] = (0, 0)
    v = position("gate_wall")
    check("gate_side_blocks", 3000 < v.x < 3030)
    place(-200, 0)
    state["input"] = (0, -1)
    yield delay(2)
    state["input"] = (0, 0)
    v = position("outer_boundary")
    check("boundary_retains_player", -460 < v.x < -420 and 80 < v.z < 105)
    place(2500, 700)
    yield delay(.3)
    pc.get_hud().toggle_inventory()
    check("inventory_pauses_world", unreal.GameplayStatics.is_game_paused(world))
    before = pawn.get_actor_location()
    state["input"] = (0, 1)
    yield delay(1)
    check("paused_player_stays", (pawn.get_actor_location() - before).length() < 1)
    pc.get_hud().toggle_inventory()
    check("inventory_resumes_world", not unreal.GameplayStatics.is_game_paused(world))
    yield delay(1)
    state["input"] = (0, 0)
    check("movement_resumes", pawn.get_actor_location().x > before.x + 200)
    capture("landmark")
    yield delay(.5)
    state.pop("pawn")
    levels.editor_request_end_play()
    yield wait(lambda: not levels.is_in_play_in_editor())
    yield delay(1)
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world, 0)))
    yield delay(1)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    v = pawn.get_actor_location()
    check("second_pie_fresh_spawn", abs(v.x) < 5 and abs(v.y) < 5 and 80 < v.z < 105)


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    (out / "verification.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


flow = run()
pending = None
started = time.monotonic()


def tick(dt):
    global pending
    try:
        if time.monotonic() - started > 180:
            raise TimeoutError("Graybox validation timed out")
        if state.get("pawn"):
            x, y = state["input"]
            state["sub"].inject_input_vector_for_action(state["move"], unreal.Vector(x, y, 0), [], [])
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("PIE wait timed out")
                return
        pending = next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
