"""TASK-050 PIE: a saved new campaign starts the W conversation clock at first meeting."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/pie"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE new progress, save/load before first meeting"}
st = {}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=20):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def finish(error=None):
    if error:
        report["error"] = error
    if "save" in st:
        report["save_status"] = st["save"].get_status()
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    (out / "results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


def run():
    editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor = editor.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(1600, 1600, 1))
    floor.static_mesh_component.set_collision_profile_name("BlockAll")
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(1)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    unreal.GameplayStatics.set_game_paused(world, False)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    yield delay(.5)
    brother = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    game = player.get_component_by_class(unreal.HearthwardGameplayComponent)
    game.enable_adventure()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    save = subsystem(unreal.HearthwardSaveSubsystem, world)
    clock = subsystem(unreal.HearthwardWorldClockSubsystem, world)
    ai = subsystem(unreal.HearthwardLocalAISubsystem, world)
    st.update(world=world, player=player, brother=brother, save=save, ai=ai)
    check("prototype enabled", save.enable_prototype())
    check("new progress", save.start_new_progress())
    player.set_actor_location(unreal.Vector(5000, 5000, 100), False, True)
    brother.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    brother.camp.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    bag = player.get_component_by_class(unreal.HearthwardInventoryComponent)
    check("food fixture", bag.try_add("wild_food", 30) == unreal.HearthwardInventoryResult.SUCCESS)
    for index in range(9):
        check("separated W " + str(index), clock.advance_calendar(480) > 479)
        for _ in range(3):
            game.use_item("wild_food")
        yield delay(.1)
    check("no remote reminder", not ai.has_active_initiative())
    existing = {key(point.save_id) for point in save.get_points()}
    check("save before meeting", save.save_point(True))
    created = [point.save_id for point in save.get_points() if key(point.save_id) not in existing]
    check("one pre-meeting point", len(created) == 1)
    check("reload before meeting", save.load_point(created[0]))
    unreal.GameplayStatics.set_game_paused(world, False)
    player.set_actor_location(unreal.Vector(-150, -150, 100), False, True)
    yield delay(1)
    check("first meeting after three days has no immediate reminder", not ai.has_active_initiative())
    finish()


flow = run()
pending = None


def tick(_delta):
    global pending
    try:
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("PIE step timed out")
                return
        pending = next(flow)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
