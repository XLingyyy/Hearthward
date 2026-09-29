"""PIE: an assigned companion runs one real limited camp production batch."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/camp_batch"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE assigned workbench region, real shared exchange, active reload and bounded completion"}
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
    return json.loads(st["camp"].describe())


def region():
    return next(x for x in state()["regions"] if x["facility"] == st["facility_id"])


def guid(text):
    value = unreal.GuidLibrary.parse_string_to_guid(text)
    return value[0] if isinstance(value, tuple) else value


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


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
        report["region"] = region()
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
    st.update(world=world, player=player, pc=pc, brother=brother,
              bag=player.get_component_by_class(unreal.HearthwardInventoryComponent))
    for name, cls in (("camp", unreal.HearthwardCampSubsystem), ("store", unreal.HearthwardStorageSubsystem),
                      ("save", unreal.HearthwardSaveSubsystem), ("ai", unreal.HearthwardLocalAISubsystem)):
        st[name] = subsystem(cls, world)
    check("prototype enabled", st["save"].enable_prototype())
    check("new progress", st["save"].start_new_progress())
    player.set_actor_location(unreal.Vector(-200, -200, 100), False, True)
    yield delay(1)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    access = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    for part in range(10):
        check("supply wood " + str(part), st["bag"].try_add("wood", 8) == unreal.HearthwardInventoryResult.SUCCESS)
        receipt = access.transfer(st["bag"], True, "wood", 8,
                                  unreal.GuidLibrary.new_guid(), st["store"].get_timeline_epoch())
        check("deposit wood " + str(part), receipt.moved_count == 8)
    builder = player.get_component_by_class(unreal.HearthwardBuildingComponent)
    check("workbench choice", builder.select_building("workbench"))
    yield delay(.4)
    check("workbench built", builder.confirm_placement())
    yield wait(lambda: not builder.is_building(), 10)
    facility = next(x for x in state()["facilities"] if x["kind"] == "workbench")
    st["facility_id"] = facility["id"]
    station = builder.get_buildings()[0]
    spot = station.get_actor_location()
    player.set_actor_location(spot + unreal.Vector(-180, 0, 80), False, True)
    brother.set_actor_location(spot + unreal.Vector(-130, 0, 80), False, True)
    yield delay(.8)
    rid = region()["id"]
    epoch = st["store"].get_timeline_epoch()
    check("configure real rope region", st["camp"].select_production(rid, guid(st["facility_id"]), "rope", epoch))
    check("assign brother only", st["camp"].assign_worker(rid, 31, epoch))
    check("queue initially paused", not region()["enabled"] and region()["brother"] and not region()["workers"])
    before_wood = st["store"].get_item_count("wood")
    before_rope = st["store"].get_item_count("rope")
    before_completed = region()["completed"]
    ui = pc.get_hud().get_editor_property("screen")
    ui.open_page("camp")
    check("workers tab", ui.execute_action("camp.tab:workers"))
    offered = False
    for _ in range(len(state()["regions"]) + 1):
        if ui.execute_action("camp.brotherBatch:1"):
            offered = True
            break
        ui.execute_action("camp.regionNext")
    check("specific region UI card", offered)
    check("region confirmed", st["ai"].confirm_candidate(st["ai"].get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.CAMP_BATCH_WORKING
               and region()["batchStopAt"] == before_completed + 1, 20)
    check("one batch authorized", region()["enabled"] and brother.get_delivered() == 0)
    yield wait(lambda: not player.get_movement_component().is_falling()
               and not brother.get_movement_component().is_falling(), 8)
    active_save = save_point("save active batch")
    check("reload active batch", st["save"].load_point(active_save))
    check("active limit survives reload", region()["batchStopAt"] == before_completed + 1)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 160)
    check("one actual batch completed", region()["completed"] == before_completed + 1
          and not region()["enabled"] and not region()["batch"]["active"])
    check("real shared stock exchanged once", st["store"].get_item_count("wood") == before_wood - 2
          and st["store"].get_item_count("rope") == before_rope + 1)
    check("one batch receipt", brother.get_acquired() == 1 and brother.get_delivered() == 1)
    yield delay(2)
    check("no unapproved follow-on batch", region()["completed"] == before_completed + 1
          and st["store"].get_item_count("wood") == before_wood - 2)
    completed_save = save_point("save completed batch")
    check("reload completed batch", st["save"].load_point(completed_save))
    check("completion not duplicated", region()["completed"] == before_completed + 1
          and brother.get_delivered() == 1 and st["store"].get_item_count("rope") == before_rope + 1)
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
