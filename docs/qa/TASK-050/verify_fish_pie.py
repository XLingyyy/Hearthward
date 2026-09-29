"""Focused PIE for one accompanied companion catch through the Nature fish ledger."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/fish"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE designated fish point, real bait/rod/fish and save"}
st = {}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=35):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def state():
    return json.loads(st["nature"].describe())


def point():
    return next(x for x in state()["points"] if x["id"] == st["point_id"])


def guid(value):
    parsed = unreal.GuidLibrary.parse_string_to_guid(value)
    return parsed[0] if isinstance(parsed, tuple) else parsed


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def rod_durability(rod_id):
    return next(x["durability"] for x in json.loads(st["brother"].bag.describe_inventory())["instances"]
                if key(guid(x["id"])) == key(rod_id))


def fish_count():
    return sum(st["brother"].bag.get_item_count(item) for item in
               ("fish_carp", "fish_crucian_carp", "fish_catfish", "fish_eel"))


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["point"] = point()
        report["phase"] = str(st["brother"].get_phase())
        report["block_reason"] = str(st["brother"].block_reason)
        report["action"] = st["brother"].get_execution_action()
        report["nature_feedback"] = str(st["nature"].feedback)
        report["save_status"] = st["save"].get_status()
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
    floor.tags = ["Hearthward.NatureGround"]
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
    player.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    save = subsystem(unreal.HearthwardSaveSubsystem, world)
    check("prototype enabled", save.enable_prototype())
    check("new progress", save.start_new_progress())
    st.update(world=world, player=player, brother=brother, save=save,
              nature=subsystem(unreal.HearthwardNatureSubsystem, world),
              ai=subsystem(unreal.HearthwardLocalAISubsystem, world))
    yield wait(lambda: any(x["kind"] == "fish" and x["remaining"] > 0 for x in state()["points"]), 12)
    target = next(x for x in state()["points"] if x["kind"] == "fish" and x["remaining"] > 0)
    st["point_id"] = target["id"]
    place = target["position"]
    player.set_actor_location(unreal.Vector(place["x"] - 120, place["y"], place["z"] + 100), False, True)
    brother.set_actor_location(unreal.Vector(place["x"] - 180, place["y"], place["z"] + 100), False, True)
    yield delay(.5)
    bag = brother.bag
    check("supply bait", bag.try_add("bait", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    check("supply rod", bag.try_add("fishing_rod", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    rod = bag.first_instance("fishing_rod")
    before_rod = rod_durability(rod)
    before_fish = fish_count()
    before_remaining = point()["remaining"]
    before_successes = point()["successes"]
    goal = unreal.HearthwardAgentGoal()
    for field, value in {"intent": "fish", "item": "fish", "quantity": 1,
                         "quantity_mode": "one_catch", "source_ref": "known_target",
                         "station": guid(target["id"])}.items():
        goal.set_editor_property(field, value)
    check("specific fish point card", st["ai"].set_structured_goal(player, brother, goal))
    check("fish confirmation", st["ai"].confirm_candidate(st["ai"].get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("one real fish in brother bag", fish_count() == before_fish + 1)
    check("one bait consumed", bag.get_item_count("bait") == 0)
    check("rod actually worn", rod_durability(rod) < before_rod)
    check("one fish stock debited", point()["remaining"] == before_remaining - 1
          and point()["successes"] == before_successes + 1)
    check("catch receipt", brother.get_acquired() == 1 and brother.get_delivered() == 1)
    yield wait(lambda: not player.get_movement_component().is_falling()
               and not brother.get_movement_component().is_falling(), 8)
    before_saves = {key(x.save_id) for x in save.get_points()}
    check("save catch", save.save_point(True))
    added = [x.save_id for x in save.get_points() if key(x.save_id) not in before_saves]
    check("catch save point", len(added) == 1)
    check("reload catch", save.load_point(added[0]))
    check("no duplicated fish or stock after reload", fish_count() == before_fish + 1
          and point()["remaining"] == before_remaining - 1 and brother.get_delivered() == 1)
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
                    raise TimeoutError("phase=" + str(st.get("brother").get_phase())
                                       + " block=" + str(st.get("brother").block_reason))
                return
        pending = next(flow)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
