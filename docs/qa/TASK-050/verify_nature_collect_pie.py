"""Focused PIE for a known camp resource point and real companion delivery."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/naturecollect"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE Nature point, companion action, camp transfer and save"}
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


def guid(value):
    result = unreal.GuidLibrary.parse_string_to_guid(value)
    return result[0] if isinstance(result, tuple) else result


def guid_key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def source_stock(key):
    camp = json.loads(st["camp"].describe())
    return next(x["remaining"] for x in camp["sources"] if x["id"] == key)


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["phase"] = str(st["brother"].get_phase())
        report["block_reason"] = str(st["brother"].block_reason)
        report["action"] = st["brother"].get_execution_action()
        report["brother_position"] = str(st["brother"].get_actor_location())
        report["player_position"] = str(st["player"].get_actor_location())
        report["target"] = st.get("target")
        report["camp_actor_position"] = str(st["brother"].camp.get_actor_location())
        report["nature"] = json.loads(st["nature"].describe())
        report["camp"] = json.loads(st["camp"].describe())
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
    game = player.get_component_by_class(unreal.HearthwardGameplayComponent)
    game.enable_adventure()
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent) and not isinstance(actor, unreal.HearthwardNatureActor):
            actor.set_actor_location(unreal.Vector(150000, 150000, 500), False, True)
    save = subsystem(unreal.HearthwardSaveSubsystem, world)
    check("prototype enabled", save.enable_prototype())
    check("new progress", save.start_new_progress())
    ai = subsystem(unreal.HearthwardLocalAISubsystem, world)
    store = subsystem(unreal.HearthwardStorageSubsystem, world)
    camp = subsystem(unreal.HearthwardCampSubsystem, world)
    nature = subsystem(unreal.HearthwardNatureSubsystem, world)
    st.update(world=world, player=player, brother=brother, ai=ai, store=store,
              camp=camp, nature=nature, save=save)
    yield wait(lambda: any(x["definition"] == "loose_stones" for x in json.loads(nature.describe())["points"]), 10)
    camp_state = json.loads(camp.describe())
    site = camp_state["camps"][0]["position"]
    sources = {x["id"]: x for x in camp_state["sources"]}
    point = next(x for x in json.loads(nature.describe())["points"]
                 if x["definition"] == "loose_stones" and x["key"] in sources
                 and sources[x["key"]]["remaining"] >= 3 and not sources[x["key"]]["blocked"])
    st["target"] = point
    position = point["position"]
    x, y, z = position["x"], position["y"], position["z"]
    player.set_actor_location(unreal.Vector(x - 100, y, z + 100), False, True)
    brother.set_actor_location(unreal.Vector(x - 180, y, z + 100), False, True)
    brother.camp.set_actor_location(unreal.Vector(x + (site["x"] - x) * .2,
                                                  y + (site["y"] - y) * .2, z + 100), False, True)
    yield delay(.5)
    before_source = source_stock(point["key"])
    before_store = store.get_item_count("stone")
    before_bag = brother.bag.get_item_count("stone")
    ui.open_nature(guid(point["id"]))
    check("resource UI card", ui.execute_action("nature.brother_collect:3"))
    check("resource confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 55)
    check("three units actually harvested", source_stock(point["key"]) == before_source - 3)
    check("three units actually delivered", store.get_item_count("stone") == before_store + 3)
    check("no command cargo left in bag", brother.bag.get_item_count("stone") == before_bag)
    check("separate acquired and delivered totals", brother.get_acquired() == 3 and brother.get_delivered() == 3)
    acquired = [event.count for event in ai.get_events() if str(event.kind) == "acquired" and str(event.item) == "stone"]
    delivered = [event.count for event in ai.get_events() if str(event.kind) == "delivered" and str(event.item) == "stone"]
    check("two physical resource and delivery receipts", sorted(acquired) == [1, 2] and sorted(delivered) == [1, 2])
    ui.open_page("hud")
    existing = {guid_key(x.save_id) for x in save.get_points()}
    check("save completed collection", save.save_point(True))
    created = [x.save_id for x in save.get_points() if guid_key(x.save_id) not in existing]
    check("new save point", len(created) == 1)
    check("reload completed collection", save.load_point(created[0]))
    check("reload does not duplicate source or output", source_stock(point["key"]) == before_source - 3
          and store.get_item_count("stone") == before_store + 3)

    # Existing player-to-brother transfer supplies a separate, explicitly authorized camp delivery.
    player_bag = player.get_component_by_class(unreal.HearthwardInventoryComponent)
    brother_at = brother.get_actor_location()
    player.set_actor_location(unreal.Vector(brother_at.x - 100, brother_at.y, brother_at.z + 10), False, True)
    check("player handoff supply", player_bag.try_add("wood", 4) == unreal.HearthwardInventoryResult.SUCCESS)
    player_before = player_bag.get_item_count("wood")
    brother_before = brother.bag.get_item_count("wood")
    camp_before = store.get_item_count("wood")
    handoff = unreal.HearthwardAgentGoal()
    for key, field in {"intent": "store", "item": "wood", "quantity": 4,
                       "quantity_mode": "held_to_camp", "source_ref": "player_bag"}.items():
        handoff.set_editor_property(key, field)
    check("player handoff card", ai.set_structured_goal(player, brother, handoff))
    check("card identifies player bag", "玩家背包" in ai.get_candidate_text())
    check("player handoff confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    check("four items physically handed over", player_bag.get_item_count("wood") == player_before - 4
          and brother.bag.get_item_count("wood") == brother_before + 4)
    check("player handoff has its own provenance", any(str(event.kind) == "handoff" and event.count == 4
                                                   for event in ai.get_events()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 30)
    check("player cargo delivered once", store.get_item_count("wood") == camp_before + 4
          and brother.bag.get_item_count("wood") == brother_before and brother.get_delivered() == 4)
    ui.open_page("hud")
    existing = {guid_key(x.save_id) for x in save.get_points()}
    check("save handoff", save.save_point(True))
    created = [x.save_id for x in save.get_points() if guid_key(x.save_id) not in existing]
    check("new handoff save point", len(created) == 1)
    check("reload handoff", save.load_point(created[0]))
    check("reload does not duplicate handoff", player_bag.get_item_count("wood") == player_before - 4
          and store.get_item_count("wood") == camp_before + 4)
    brother_at = brother.get_actor_location()
    dx, dy = site["x"] - brother_at.x, site["y"] - brother_at.y
    length = (dx * dx + dy * dy) ** .5
    player.set_actor_location(unreal.Vector(brother_at.x + dx / length * 400,
                                            brother_at.y + dy / length * 400, brother_at.z + 10), False, True)
    check("distant player bag cannot be handed over", not ai.set_structured_goal(player, brother, handoff))
    check("rejected handoff leaves both containers unchanged", player_bag.get_item_count("wood") == player_before - 4
          and store.get_item_count("wood") == camp_before + 4)
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
