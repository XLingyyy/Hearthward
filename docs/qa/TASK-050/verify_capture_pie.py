"""PIE: designated domestic animal follows the companion into a real pen."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/capture"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE designated goat, companion capture, physical pen arrival and save"}
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


def guid(text):
    value = unreal.GuidLibrary.parse_string_to_guid(text)
    return value[0] if isinstance(value, tuple) else value


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def position(row):
    p = row["position"]
    return unreal.Vector(p["x"], p["y"], p["z"])


def go(place):
    st["ui"].open_page("hud")
    st["player"].set_actor_location(place + unreal.Vector(-150, 0, 120), False, True)


def grant_shared(item, amount):
    while amount:
        part = min(amount, 8)
        check("supply " + item, st["bag"].try_add(item, part) == unreal.HearthwardInventoryResult.SUCCESS)
        receipt = st["access"].transfer(st["bag"], True, item, part,
                                         unreal.GuidLibrary.new_guid(), st["store"].get_timeline_epoch())
        check("deposit " + item, receipt.moved_count == part)
        amount -= part


def save_point(label):
    old = {key(point.save_id) for point in st["save"].get_points()}
    check(label, st["save"].save_point(True))
    created = [point.save_id for point in st["save"].get_points() if key(point.save_id) not in old]
    check(label + " identity", len(created) == 1)
    return created[0]


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
    game.grant_initial_equipment()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    st.update(world=world, player=player, pc=pc, brother=brother, game=game,
              ui=pc.get_hud().get_editor_property("screen"),
              bag=player.get_component_by_class(unreal.HearthwardInventoryComponent))
    for name, cls in (("nature", unreal.HearthwardNatureSubsystem), ("camp", unreal.HearthwardCampSubsystem),
                      ("store", unreal.HearthwardStorageSubsystem), ("save", unreal.HearthwardSaveSubsystem),
                      ("ai", unreal.HearthwardLocalAISubsystem)):
        st[name] = subsystem(cls, world)
    check("prototype enabled", st["save"].enable_prototype())
    check("new progress", st["save"].start_new_progress())
    go(unreal.Vector(-200, -200, 0))
    yield delay(1)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    st["access"] = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    for item, amount in (("wood", 240), ("stone", 90), ("rope", 20)):
        grant_shared(item, amount)
    builder = player.get_component_by_class(unreal.HearthwardBuildingComponent)
    check("workbench choice", builder.select_building("workbench"))
    yield delay(.4)
    check("workbench built", builder.confirm_placement())
    yield wait(lambda: not builder.is_building(), 10)
    check("rescue prerequisite", st["camp"].record_rescue("task050_capture_fixture"))
    check("tier two", st["camp"].upgrade_camp(st["store"].get_timeline_epoch()))
    go(unreal.Vector(-2300, -2200, 0))
    yield delay(1)
    st["ui"].open_nature(unreal.Guid())
    check("start goat pen", st["ui"].execute_action("nature.build_pen:goat"))
    yield wait(lambda: not st["nature"].busy(), 10)
    check("real goat pen", str(st["nature"].feedback) == "操作完成")
    pen = state()["pens"][0]
    goat = next(a for a in state()["animals"] if a["domestic"] and a["definition"] == "goat")
    st["animal_id"] = goat["id"]
    actor = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardNatureActor)
                 if str(a.get_component_by_class(unreal.HearthwardCombatTargetComponent).get_editor_property("id")) == goat["id"])
    actor.set_actor_tick_enabled(False)
    animal_place = position(pen) + unreal.Vector(-850, 0, 100)
    actor.set_actor_location(animal_place, False, True)
    go(animal_place)
    brother.set_actor_location(animal_place + unreal.Vector(-200, 0, 0), False, True)
    yield delay(.8)
    check("brother feed", brother.bag.try_add("feed", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    check("brother rope", brother.bag.try_add("rope", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    actor.set_actor_tick_enabled(True)
    goal = unreal.HearthwardAgentGoal()
    for field, value in {"intent": "capture", "item": "goat", "quantity": 1,
                         "quantity_mode": "one_animal", "source_ref": "known_target",
                         "station": guid(goat["id"])}.items():
        goal.set_editor_property(field, value)
    check("designated capture card", st["ai"].set_structured_goal(player, brother, goal))
    check("capture confirmed", st["ai"].confirm_candidate(st["ai"].get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.GATHERING
               and brother.action.get_status() == unreal.HearthwardTimedActionStatus.RUNNING, 12)
    held = actor.get_actor_location()
    yield delay(1)
    moving = actor.get_actor_location()
    check("live animal held during capture action", abs(moving.x - held.x) < 1 and abs(moving.y - held.y) < 1)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.LEADING_ANIMAL, 25)
    check("capture debits material once", brother.bag.get_item_count("feed") == 0
          and brother.bag.get_item_count("rope") == 0)
    check("capture not credited before arrival", brother.get_delivered() == 0
          and animal()["reservedPen"] == pen["id"] and animal()["followingBrother"])
    yield wait(lambda: not player.get_movement_component().is_falling()
               and not brother.get_movement_component().is_falling(), 8)
    active_save = save_point("save active牵引")
    check("reload active牵引", st["save"].load_point(active_save))
    check("active capture resumes same animal", brother.get_phase() == unreal.HearthwardCompanionPhase.LEADING_ANIMAL
          and brother.get_delivered() == 0 and animal()["reservedPen"] == pen["id"]
          and animal()["followingBrother"] and brother.bag.get_item_count("rope") == 0)
    actor = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardNatureActor)
                 if str(a.get_component_by_class(unreal.HearthwardCombatTargetComponent).get_editor_property("id")) == goat["id"])
    actor.set_actor_tick_enabled(True)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("animal physically in pen", animal()["pen"] == pen["id"] and not animal()["following"])
    check("single arrival receipt", brother.get_acquired() == 1 and brother.get_delivered() == 1)
    yield wait(lambda: not player.get_movement_component().is_falling()
               and not brother.get_movement_component().is_falling(), 8)
    saved = save_point("save captured animal")
    check("reload capture", st["save"].load_point(saved))
    check("no duplicate capture after reload", animal()["pen"] == pen["id"]
          and brother.get_delivered() == 1 and brother.bag.get_item_count("rope") == 0)
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
