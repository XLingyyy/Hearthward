import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/facility_craft"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE non-workbench companion recipe via real facility and inventory service"}
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


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["phase"] = str(st["brother"].get_phase())
        report["reason"] = str(st["brother"].block_reason)
        report["facilities"] = json.loads(st["camp"].describe())["facilities"]
        report["camp_roast"] = st["store"].get_item_count("roast")
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
    player.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
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
    builder = player.get_component_by_class(unreal.HearthwardBuildingComponent)
    st.update(player=player, brother=brother, camp=camp, store=store)
    site = json.loads(camp.describe())["camps"][0]["position"]
    brother.camp.set_actor_location(unreal.Vector(site["x"] + 100, site["y"], 50), False, True)
    player.set_actor_location(unreal.Vector(site["x"] - 200, site["y"] - 200, 100), False, True)
    brother.set_actor_location(unreal.Vector(site["x"] - 100, site["y"] + 200, 100), False, True)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    access = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    for item, count in (("wood", 4), ("stone", 4)):
        check("material supply " + item, bag.try_add(item, count) == unreal.HearthwardInventoryResult.SUCCESS)
        moved = access.transfer(bag, True, item, count, unreal.GuidLibrary.new_guid(), store.get_timeline_epoch())
        check("construction stock " + item, moved.moved_count == count)
    pc.set_control_rotation(unreal.Rotator(pitch=-20, yaw=0))
    yield delay(.6)
    check("campfire selected", builder.select_building("campfire"))
    yield delay(.3)
    check("campfire placement", builder.confirm_placement())
    yield wait(lambda: builder.building_count() >= 1, 12)
    check("campfire registered", any(f["kind"] == "campfire" for f in json.loads(camp.describe())["facilities"]))
    fire = builder.get_buildings()[0]
    at = fire.get_actor_location()
    brother.set_actor_location(at + unreal.Vector(250, 0, 100), False, True)
    player.set_actor_location(at + unreal.Vector(150, 100, 100), False, True)
    check("brother recipe materials", brother.bag.try_add("meat", 1) == unreal.HearthwardInventoryResult.SUCCESS
          and brother.bag.try_add("wood", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    before = store.get_item_count("roast")
    goal = unreal.HearthwardAgentGoal()
    for name, value in {"intent": "craft", "item": "roast", "quantity": 1,
                        "quantity_mode": "batches", "source_ref": "bag"}.items():
        goal.set_editor_property(name, value)
    check("campfire recipe card", ai.set_structured_goal(player, brother, goal))
    check("campfire recipe confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("non-workbench recipe commits once and deposits", brother.get_delivered() == 1
          and store.get_item_count("roast") == before + 1
          and brother.bag.get_item_count("meat") == 0 and brother.bag.get_item_count("roast") == 0)
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
