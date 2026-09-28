import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/brother_work"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE brother resumes real camp labor after a completed task"}
st = {}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=30):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def state():
    return json.loads(st["camp"].describe())


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["phase"] = str(st["brother"].get_phase())
        report["goal"] = str(st["brother"].get_goal().intent)
        report["region"] = next(r for r in state()["regions"] if r["id"] == "camp_forage")
        report["wild_food"] = st["store"].get_item_count("wild_food")
    except Exception:
        report["observer_error"] = traceback.format_exc()
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
    gameplay = player.get_component_by_class(unreal.HearthwardGameplayComponent)
    gameplay.enable_adventure()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    save = subsystem(unreal.HearthwardSaveSubsystem, world)
    check("prototype enabled", save.enable_prototype())
    check("new progress", save.start_new_progress())
    camp = subsystem(unreal.HearthwardCampSubsystem, world)
    store = subsystem(unreal.HearthwardStorageSubsystem, world)
    ai = subsystem(unreal.HearthwardLocalAISubsystem, world)
    bag = player.get_component_by_class(unreal.HearthwardInventoryComponent)
    st.update(player=player, brother=brother, camp=camp, store=store)
    site = state()["camps"][0]["position"]
    at = unreal.Vector(site["x"] + 400, site["y"], 100)
    brother.camp.set_actor_location(unreal.Vector(site["x"] + 100, site["y"], 50), False, True)
    brother.set_actor_location(at, False, True)
    player.set_actor_location(at + unreal.Vector(150, 0, 0), False, True)
    check("player supplies one wood", bag.try_add("wood", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    goal = unreal.HearthwardAgentGoal()
    for name, value in {"intent": "receive", "item": "wood", "quantity": 1,
                        "quantity_mode": "player_to_bag", "source_ref": "player_bag"}.items():
        goal.set_editor_property(name, value)
    check("receive card", ai.set_structured_goal(player, brother, goal))
    check("receive confirmed", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 20)
    check("prior task completed and remains in status", brother.get_delivered() == 1
          and str(brother.get_goal().intent) == "receive")
    check("hold after completed task", gameplay.apply_companion_directive(player, "hold"))
    check("forage source registered", camp.register_source("task050_brother_work", "wild_food", 10, 10, at, 2880))
    check("brother assigned to forage", camp.assign_worker("camp_forage", 31, store.get_timeline_epoch()))
    check("forage enabled", camp.set_production("camp_forage", True, False, store.get_timeline_epoch()))
    before = next(r for r in state()["regions"] if r["id"] == "camp_forage")["batch"]["work"]
    yield delay(1.5)
    region = next(r for r in state()["regions"] if r["id"] == "camp_forage")
    check("completed task does not permanently suppress assigned work", region["batch"]["active"]
          and region["batch"]["work"] > before + 1 and not region["workers"]
          and region["brother"])
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
                    raise TimeoutError("phase=" + str(st.get("brother").get_phase()))
                return
        pending = next(flow)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
