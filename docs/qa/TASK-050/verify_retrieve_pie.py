"""PIE proof for real camp withdrawal, player handoff and cargo-safe recovery."""
import json
import math
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/retrieve"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE camp-to-player cargo, save and return"}
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


def key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def point(x, y=0):
    site = st["site"]
    return unreal.Vector(site["x"] + x, site["y"] + y, 100)


def grant(count):
    bag, access, store = st["bag"], st["access"], st["store"]
    check("player supply " + str(count), bag.try_add("wood", count) == unreal.HearthwardInventoryResult.SUCCESS)
    moved = access.transfer(bag, True, "wood", count, unreal.GuidLibrary.new_guid(), store.get_timeline_epoch())
    check("camp supply " + str(count), moved.moved_count == count)


def order(count, intent="retrieve"):
    st["orders"] = st.get("orders", 0) + 1
    label = f"{intent} {count} #{st['orders']}"
    goal = unreal.HearthwardAgentGoal()
    for name, value in {"intent": intent, "item": "wood", "quantity": count,
                        "quantity_mode": "bag_to_player" if intent == "give" else "camp_to_player",
                        "source_ref": "bag" if intent == "give" else "camp"}.items():
        goal.set_editor_property(name, value)
    ai, player, brother = st["ai"], st["player"], st["brother"]
    check("delivery card " + label, ai.set_structured_goal(player, brother, goal))
    check("card shows player destination " + label, "玩家背包" in ai.get_candidate_text())
    check("confirm delivery " + label, ai.confirm_candidate(ai.get_candidate_id()))


def save_point(label):
    save = st["save"]
    st["ui"].open_page("hud")
    before = {key(x.save_id) for x in save.get_points()}
    saved = save.save_point(True)
    report[label + "_save_status"] = save.get_status()
    check(label + " save carrying cargo", saved)
    added = [x.save_id for x in save.get_points() if key(x.save_id) not in before]
    check(label + " save point created", len(added) == 1)
    return added[0]


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["phase"] = str(st["brother"].get_phase())
        report["reason"] = str(st["brother"].block_reason)
        report["action"] = st["brother"].get_execution_action()
        report["delivered"] = st["brother"].get_delivered()
        report["carried"] = st["brother"].get_carried()
        report["camp_wood"] = st["store"].get_item_count("wood")
        report["player_wood"] = st["bag"].get_item_count("wood")
        report["player_position"] = str(st["player"].get_actor_location())
        report["brother_position"] = str(st["brother"].get_actor_location())
        report["camp_position"] = str(st["brother"].camp.get_actor_location())
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
    ui = pc.get_hud().get_editor_property("screen")
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
    ai = subsystem(unreal.HearthwardLocalAISubsystem, world)
    store = subsystem(unreal.HearthwardStorageSubsystem, world)
    camp = subsystem(unreal.HearthwardCampSubsystem, world)
    bag = player.get_component_by_class(unreal.HearthwardInventoryComponent)
    st.update(world=world, player=player, brother=brother, ai=ai, store=store,
              camp=camp, bag=bag, save=save, ui=ui)
    st["site"] = json.loads(camp.describe())["camps"][0]["position"]
    brother.camp.set_actor_location(unreal.Vector(st["site"]["x"] + 100, st["site"]["y"], 50), False, True)
    brother.set_actor_location(point(300, -100), False, True)
    player.set_actor_location(point(1000, 100), False, True)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    st["access"] = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    grant(5)
    free = math.floor(brother.bag.get_capacity() - brother.bag.get_weight())
    check("capacity fixture", free >= 7 and brother.bag.try_add("stone", free - 2) == unreal.HearthwardInventoryResult.SUCCESS)
    before_camp, before_player = store.get_item_count("wood"), bag.get_item_count("wood")
    order(5)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 100)
    check("five camp units withdrawn and handed to player", store.get_item_count("wood") == before_camp - 5
          and bag.get_item_count("wood") == before_player + 5 and brother.get_delivered() == 5)
    withdrawals = [x.count for x in ai.get_events() if str(x.kind) == "withdrawn" and str(x.item) == "wood"]
    deliveries = [x.count for x in ai.get_events() if str(x.kind) == "delivered" and str(x.item) == "wood"]
    check("capacity requires multiple physical trips", len(withdrawals) >= 2 and sum(withdrawals) == 5)
    check("only player handoffs count", sum(deliveries) == 5 and brother.get_carried() == 0)

    grant(2)
    brother.set_actor_location(point(300, -100), False, True)
    player.set_actor_location(point(1400, 100), False, True)
    second_camp, second_player = store.get_item_count("wood"), bag.get_item_count("wood")
    order(2)
    yield wait(lambda: brother.get_carried() == 2 and brother.get_phase() == unreal.HearthwardCompanionPhase.GOING_TO_PLAYER, 20)
    checkpoint = save_point("warehouse")
    check("reload carrying cargo", save.load_point(checkpoint))
    check("cargo survives reload once", brother.get_carried() == 2 and store.get_item_count("wood") == second_camp - 2)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("loaded cargo delivered once", store.get_item_count("wood") == second_camp - 2
          and bag.get_item_count("wood") == second_player + 2 and brother.get_delivered() == 2)

    grant(2)
    brother.set_actor_location(point(300, -100), False, True)
    player.set_actor_location(point(1400, 100), False, True)
    third_camp, third_player = store.get_item_count("wood"), bag.get_item_count("wood")
    order(2)
    yield wait(lambda: brother.get_carried() == 2 and brother.get_phase() == unreal.HearthwardCompanionPhase.GOING_TO_PLAYER, 20)
    player.set_actor_location(point(20000), False, True)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.WAITING_AT_CAMP, 35)
    check("player leaving camp returns undelivered cargo", store.get_item_count("wood") == third_camp
          and bag.get_item_count("wood") == third_player and brother.get_carried() == 0
          and brother.get_acquired() == 0 and brother.get_delivered() == 0)
    check("return has distinct receipt", any(str(x.kind) == "returned" and x.count == 2 for x in ai.get_events()))
    at = brother.get_actor_location()
    player.set_actor_location(unreal.Vector(at.x + 150, at.y, at.z + 10), False, True)
    check("explicit retry", brother.resume_blocked(player))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("retry handoff has no phantom delivery", store.get_item_count("wood") == third_camp - 2
          and bag.get_item_count("wood") == third_player + 2 and brother.get_delivered() == 2)

    grant(2)
    brother.set_actor_location(point(300, -100), False, True)
    player.set_actor_location(point(1400, 100), False, True)
    fourth_camp, fourth_player = store.get_item_count("wood"), bag.get_item_count("wood")
    order(2)
    yield wait(lambda: brother.get_carried() == 2 and brother.get_phase() == unreal.HearthwardCompanionPhase.GOING_TO_PLAYER, 20)
    check("simulate one consumed cargo unit", brother.bag.try_remove("wood", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    player.set_actor_location(point(20000), False, True)
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.WAITING_AT_CAMP, 35)
    check("missing cargo never becomes delivery or phantom return", store.get_item_count("wood") == fourth_camp - 1
          and bag.get_item_count("wood") == fourth_player and brother.get_acquired() == 0
          and brother.get_carried() == 0 and brother.get_delivered() == 0)
    check("missing cargo and physical return have separate receipts",
          any(str(x.kind) == "cargo_missing" and x.count == 1 for x in ai.get_events())
          and any(str(x.kind) == "returned" and x.count == 1 for x in ai.get_events()))
    at = brother.get_actor_location()
    player.set_actor_location(unreal.Vector(at.x + 150, at.y, at.z + 10), False, True)
    yield delay(.5)
    st["ui"].open_page("hud")
    saved = save.save_point(True)
    report["shortage_save_status"] = save.get_status()
    check("shortage state remains saveable", saved)

    check("supply brother bag for direct handoff",
          brother.bag.try_add("wood", 2) == unreal.HearthwardInventoryResult.SUCCESS)
    player_free = math.floor(bag.get_capacity() - bag.get_weight())
    check("player has space fixture", player_free >= 3 and
          bag.try_add("stone", player_free - 1) == unreal.HearthwardInventoryResult.SUCCESS)
    brother.set_actor_location(point(300, -100), False, True)
    player.set_actor_location(point(1400, 100), False, True)
    give_camp, give_player = store.get_item_count("wood"), bag.get_item_count("wood")
    order(2, "give")
    yield wait(lambda: brother.get_delivered() == 1 and
               brother.get_phase() == unreal.HearthwardCompanionPhase.WAITING_AT_CAMP, 40)
    check("full player bag stops after one real handoff", brother.get_carried() == 1
          and brother.bag.get_item_count("wood") == 1 and bag.get_item_count("wood") == give_player + 1
          and store.get_item_count("wood") == give_camp)
    at = brother.get_actor_location()
    player.set_actor_location(unreal.Vector(at.x + 150, at.y, at.z + 10), False, True)
    yield delay(1.5)
    checkpoint = save_point("brother")
    check("reload partial handoff", save.load_point(checkpoint))
    check("partial player handoff survives reload", brother.get_delivered() == 1
          and brother.get_carried() == 1 and brother.bag.get_item_count("wood") == 1)
    check("free space for the last handoff", bag.try_remove("stone", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    check("resume direct handoff", brother.resume_blocked(player))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 35)
    check("direct handoff completes without touching camp storage", brother.get_delivered() == 2
          and brother.get_carried() == 0 and brother.bag.get_item_count("wood") == 0
          and bag.get_item_count("wood") == give_player + 2 and store.get_item_count("wood") == give_camp)
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
