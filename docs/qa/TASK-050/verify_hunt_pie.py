"""Focused PIE for one explicitly selected wild target and actual combat settlement."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/hunt"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE designated wildlife, combat hit, corpse loot and save"}
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


def animal():
    return next(x for x in state()["animals"] if x["id"] == st["animal_id"])


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def guid(value):
    parsed = unreal.GuidLibrary.parse_string_to_guid(value)
    return parsed[0] if isinstance(parsed, tuple) else parsed


def weapon_durability(weapon_id):
    return next(x["durability"] for x in json.loads(st["brother"].bag.describe_inventory())["instances"]
                if key(guid(x["id"])) == key(weapon_id))


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["animal"] = animal()
        report["phase"] = str(st["brother"].get_phase())
        report["block_reason"] = str(st["brother"].block_reason)
        report["action"] = st["brother"].get_execution_action()
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
    game = player.get_component_by_class(unreal.HearthwardGameplayComponent)
    game.enable_adventure()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    save = subsystem(unreal.HearthwardSaveSubsystem, world)
    check("prototype enabled", save.enable_prototype())
    check("new progress", save.start_new_progress())
    st.update(world=world, player=player, brother=brother, game=game, save=save,
              nature=subsystem(unreal.HearthwardNatureSubsystem, world),
              ai=subsystem(unreal.HearthwardLocalAISubsystem, world))
    yield wait(lambda: any(x["definition"] == "hare" for x in state()["animals"]), 12)
    target = next(x for x in state()["animals"] if x["definition"] == "hare" and not x["domestic"] and x["health"] > 0)
    st["animal_id"] = target["id"]
    actor = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardNatureActor)
                 if str(a.get_component_by_class(unreal.HearthwardCombatTargetComponent).get_editor_property("id")) == target["id"])
    actor.set_actor_tick_enabled(False)
    center = player.get_actor_location()
    actor.set_actor_location(center + unreal.Vector(270, 0, 0), False, True)
    brother.set_actor_location(center + unreal.Vector(120, 0, 0), False, True)
    yield delay(.6)
    bag = brother.bag
    check("supply weapon", bag.try_add("axe", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    check("equip weapon", bag.equip_instance(bag.first_instance("axe")))
    weapon_id = bag.equipped_instance("weapon")
    before_durability = weapon_durability(weapon_id)
    before_health = animal()["health"]
    goal = unreal.HearthwardAgentGoal()
    for field, value in {"intent": "hunt", "item": "hare", "quantity": 1,
                         "quantity_mode": "one_animal", "source_ref": "known_target",
                         "station": guid(target["id"])}.items():
        goal.set_editor_property(field, value)
    check("designated hunt card", st["ai"].set_structured_goal(player, brother, goal))
    check("hunt confirmation", st["ai"].confirm_candidate(st["ai"].get_candidate_id()))
    check("alive before hit", before_health > 0 and not animal()["loot"])
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("actual target killed", animal()["health"] == 0)
    check("corpse owns real meat", animal()["loot"].get("meat", 0) > 0)
    check("real weapon wore", weapon_durability(weapon_id) < before_durability)
    check("single hunt receipt", brother.get_acquired() == 1 and brother.get_delivered() == 1)
    check("no fabricated carried meat", brother.bag.get_item_count("meat") == 0)
    yield delay(4)
    yield wait(lambda: not player.get_movement_component().is_falling()
               and not brother.get_movement_component().is_falling(), 8)
    before_saves = {key(x.save_id) for x in save.get_points()}
    check("save hunt", save.save_point(True))
    added = [x.save_id for x in save.get_points() if key(x.save_id) not in before_saves]
    check("hunt save point", len(added) == 1)
    check("reload hunt", save.load_point(added[0]))
    check("corpse loot not duplicated", animal()["health"] == 0
          and animal()["loot"].get("meat", 0) > 0 and brother.get_delivered() == 1)
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
