"""Capture untouched TASK-028 house views from the normal new-game flow."""

import json
import time
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/Task028/visual-capture.json"
SHOTS = ROOT / "Saved/Task028/visual"
SHOTS.mkdir(parents=True, exist_ok=True)
BOOTSTRAP = "/Game/Hearthward/Bootstrap/L_Bootstrap"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
report = {"ok": False, "runtime": "PIE DX12 SM6, default project scalability",
          "retouch": False, "captures": []}


def wait_for(predicate, seconds=90):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait_for(lambda: time.monotonic() >= end, seconds + 15)


def capture(world, pc, name):
    path = SHOTS / (name + ".png")
    unreal.SystemLibrary.execute_console_command(
        world, f'HighResShot 1600x900 filename="{path.as_posix()}"', pc)
    return path


def flow():
    assert levels.load_level(BOOTSTRAP)
    levels.editor_request_begin_play()
    yield wait_for(levels.is_in_play_in_editor)
    world = editor.get_game_world()
    yield wait_for(lambda: unreal.GameplayStatics.get_player_controller(world, 0))
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    screen = pc.get_hud().get_editor_property("screen")
    assert screen.execute_action("new")
    yield wait_for(lambda: editor.get_game_world() and
                   unreal.GameplayStatics.get_current_level_name(editor.get_game_world(), True) == "L_HearthwardWilds", 180)
    world = editor.get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    houses = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardTask028CampHouse)
    assert len(houses) == 1
    house = houses[0]
    p = house.get_actor_location()
    report["house_cm"] = [p.x, p.y, p.z]
    report["liner_materials"] = {
        mesh.get_name(): (mesh.get_material(0).get_path_name() if mesh.get_material(0) else None)
        for mesh in house.get_components_by_class(unreal.StaticMeshComponent)
        if mesh.get_name() in {"FloorLiner", "RearRoofLiner", "FrontRoofLiner",
                               "BackBaseboard", "LeftBaseboard", "RightBaseboard"}
    }
    # Task-only camera setup; runtime QA separately verifies walking with normal collision.
    pawn.set_actor_enable_collision(False)
    pawn.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FLYING)
    boom = pawn.get_component_by_class(unreal.SpringArmComponent)
    boom.set_editor_property("target_arm_length", 0.0)
    boom.set_editor_property("target_offset", unreal.Vector(0, 0, 0))
    for mesh in pawn.get_components_by_class(unreal.StaticMeshComponent):
        mesh.set_visibility(False)
    views = [
        ("house-exterior", unreal.Vector(p.x + 1000, p.y - 400, p.z + 290), -7, 158),
        ("house-entry", unreal.Vector(p.x + 560, p.y - 136, p.z + 170), 0, 180),
        ("house-interior", unreal.Vector(p.x + 100, p.y - 136, p.z + 175), 0, 180),
    ]
    for name, location, pitch, yaw in views:
        pawn.character_movement.stop_movement_immediately()
        pawn.set_actor_location(location, False, True)
        pc.set_control_rotation(unreal.Rotator(pitch=pitch, yaw=yaw, roll=0))
        yield delay(5)
        path = capture(world, pc, name)
        yield wait_for(lambda: path.exists() and path.stat().st_size > 10000, 90)
        actual = pawn.get_actor_location()
        report["captures"].append({"file": str(path.relative_to(ROOT)), "bytes": path.stat().st_size,
                                   "observation_camera_cm": [location.x, location.y, location.z],
                                   "actual_pawn_cm": [actual.x, actual.y, actual.z],
                                   "pitch": pitch, "yaw": yaw})
    levels.editor_request_end_play()
    yield wait_for(lambda: not levels.is_in_play_in_editor(), 30)
    report["ok"] = True


def finish(error=None):
    if error:
        report["error"] = error
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    unreal.unregister_slate_post_tick_callback(handle)


steps = flow()
pending = None


def tick(_delta):
    global pending
    try:
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("visual capture stage timeout")
                return
        pending = next(steps)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
