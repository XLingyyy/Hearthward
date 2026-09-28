"""TASK-050 focused PIE: physical cargo, nature settlement, and W reminder."""
import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task050/pie"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "checks": {}, "method": "PIE components and UI; flat test floor, explicit supplies"}
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


def subsystem(cls, world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world)


def nature_state():
    return json.loads(st["nature"].describe())


def inventory_instances(item):
    state = json.loads(st["brother"].bag.describe_inventory())
    return [entry for entry in state["instances"] if entry["definition"] == item]


def guid_key(value):
    return tuple(value.get_editor_property(part) for part in ("a", "b", "c", "d"))


def goal(intent, item, count, mode, source):
    value = unreal.HearthwardAgentGoal()
    for key, field in {"intent": intent, "item": item, "quantity": count,
                       "quantity_mode": mode, "source_ref": source}.items():
        value.set_editor_property(key, field)
    return value


def grant_shared(item, quantity):
    before = st["store"].get_item_count(item)
    requested = quantity
    while quantity:
        count = min(8, quantity)
        if st["bag"].try_add(item, count) != unreal.HearthwardInventoryResult.SUCCESS:
            raise AssertionError("fixture supply " + item + " " + str(quantity))
        receipt = st["access"].transfer(st["bag"], True, item, count,
                                        unreal.GuidLibrary.new_guid(), st["store"].get_timeline_epoch())
        if receipt.moved_count != count:
            raise AssertionError("fixture camp receipt " + item + " " + str(quantity))
        quantity -= count
    check("camp supply " + item, st["store"].get_item_count(item) == before + requested)


def save_new_point(label):
    save = st["save"]
    existing = {guid_key(point.save_id) for point in save.get_points()}
    check(label, save.save_point(True))
    created = [point.save_id for point in save.get_points() if guid_key(point.save_id) not in existing]
    check(label + " has new point", len(created) == 1)
    return created[0]


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    try:
        report["phase"] = str(st["brother"].get_phase())
        report["block_reason"] = str(st["brother"].block_reason)
        report["status"] = st["ai"].get_status()
        report["nature_feedback"] = str(st["nature"].feedback)
        report["ui_message"] = st["ui"].get_message()
        report["position"] = str(st["player"].get_actor_location())
        report["brother_position"] = str(st["brother"].get_actor_location())
        report["page"] = str(st["ui"].get_page())
        report["paused"] = unreal.GameplayStatics.is_game_paused(st["world"])
        report["velocity"] = str(st["player"].get_velocity())
        report["health"] = st["game"].health
        report["hunger"] = st["game"].hunger
        report["camp"] = json.loads(subsystem(unreal.HearthwardCampSubsystem, st["world"]).describe())
        report["nature"] = nature_state()
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
    nature = subsystem(unreal.HearthwardNatureSubsystem, world)
    clock = subsystem(unreal.HearthwardWorldClockSubsystem, world)
    bag = player.get_component_by_class(unreal.HearthwardInventoryComponent)
    st.update(world=world, player=player, pc=pc, ui=ui, brother=brother, game=game,
              save=save, ai=ai, store=store, nature=nature, clock=clock, bag=bag)

    # The selected cargo is already in the brother's physical bag.
    player.set_actor_location(unreal.Vector(-150, -150, 100), False, True)
    brother.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    brother.camp.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    check("grant retained cargo", brother.bag.try_add("wood", 5) == unreal.HearthwardInventoryResult.SUCCESS)
    source_before = brother.source.get_item_count("wood")
    stored_before = store.get_item_count("wood")
    check("store card", ai.set_structured_goal(player, brother, goal("store", "wood", 3, "held_to_camp", "bag")))
    check("store confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    check("store transfers three physical units", brother.bag.get_item_count("wood") == 2 and store.get_item_count("wood") == stored_before + 3)
    check("store does not harvest", brother.source.get_item_count("wood") == source_before)
    check("store receipt count", brother.get_delivered() == 3)
    point = save_new_point("store save")
    check("store reload", save.load_point(point))
    check("store reload does not duplicate", store.get_item_count("wood") == stored_before + 3 and brother.bag.get_item_count("wood") == 2)

    # Use the normal player planting action, then the normal brother task card.
    player.set_actor_location(unreal.Vector(-1800, -1500, 100), False, True)
    brother.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    pc.set_control_rotation(unreal.Rotator(pitch=-15, yaw=0))
    check("grant crop seed", bag.try_add("seed_greens", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    ui.open_nature(unreal.Guid())
    check("plant through UI", ui.execute_action("nature.plant:greens"))
    yield wait(lambda: not nature.busy(), 10)
    check("one crop exists", len(nature_state()["crops"]) == 1)
    crop = nature_state()["crops"][0]
    position = crop["position"]
    player.set_actor_location(unreal.Vector(position["x"] - 100, position["y"], position["z"] + 100), False, True)
    brother.set_actor_location(unreal.Vector(position["x"] - 180, position["y"], position["z"] + 100), False, True)
    yield delay(.6)
    ui.open_nature(guid(crop["id"]))
    check("brother water UI card", ui.execute_action("nature.brother_water:"))
    check("brother water confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    check("crop watered once", nature_state()["crops"][0]["watered"])
    events = [str(e.kind) for e in ai.get_events()]
    check("nature action and completion receipts", "nature_care" in events and "completed" in events)
    ui.open_nature(guid(crop["id"]))
    check("already watered rejected before confirmation", not ui.execute_action("nature.brother_water:"))
    check("duplicate has no candidate", not ai.has_candidate())
    point = save_new_point("nature save")
    check("nature reload", save.load_point(point))
    check("nature reload preserves state", nature_state()["crops"][0]["watered"])

    ui.open_nature(guid(crop["id"]))
    check("brother fertilize UI card", ui.execute_action("nature.brother_fertilize:"))
    check("brother fertilize confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    check("crop fertilized once", nature_state()["crops"][0]["fertilized"])
    check("crop time supply", bag.try_add("wild_food", 30) == unreal.HearthwardInventoryResult.SUCCESS)
    for index in range(6):
        check("crop W advance " + str(index), clock.advance_calendar(480) > 479)
        for _ in range(3):
            game.use_item("wild_food")
        yield delay(.1)
    brother.set_actor_location(unreal.Vector(position["x"] - 180, position["y"], position["z"] + 100), False, True)
    yield delay(.2)
    output_before = brother.bag.get_item_count("wild_food")
    seed_before = brother.bag.get_item_count("seed_greens")
    ui.open_nature(guid(crop["id"]))
    check("brother harvest UI card", ui.execute_action("nature.brother_harvest:"))
    check("brother harvest confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    check("harvest pays brother bag and consumes crop", not nature_state()["crops"]
          and brother.bag.get_item_count("wild_food") > output_before
          and brother.bag.get_item_count("seed_greens") == seed_before + 1)

    # Build a real pen and pay its materials through existing camp and nature services.
    ui.open_page("hud")
    player.set_actor_location(unreal.Vector(-200, -200, 100), False, True)
    brother.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
    st["access"] = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world() == world)
    for item, quantity in [("wood", 240), ("stone", 90), ("rope", 20)]:
        grant_shared(item, quantity)
    builder = player.get_component_by_class(unreal.HearthwardBuildingComponent)
    yield delay(1)
    check("workbench selected", builder.select_building("workbench"))
    yield delay(.4)
    check("workbench started", builder.confirm_placement())
    yield wait(lambda: not builder.is_building(), 10)
    check("workbench settled", builder.building_count() >= 1)
    check("first repair instance fixture", brother.bag.try_add("axe", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    first_axe = brother.bag.first_instance("axe")
    check("second repair instance fixture", brother.bag.try_add("axe", 1) == unreal.HearthwardInventoryResult.SUCCESS)
    second_row = next(x for x in inventory_instances("axe") if guid_key(guid(x["id"])) != guid_key(first_axe))
    second_axe = guid(second_row["id"])
    check("wear first repair instance", brother.bag.wear_instance(first_axe, 20))
    check("wear second repair instance", brother.bag.wear_instance(second_axe, 40))
    second_worn = next(x["durability"] for x in inventory_instances("axe")
                       if guid_key(guid(x["id"])) == guid_key(second_axe))
    check("repair material fixture", brother.bag.try_add("wood", 6) == unreal.HearthwardInventoryResult.SUCCESS
          and brother.bag.try_add("stone", 6) == unreal.HearthwardInventoryResult.SUCCESS)
    ui.open_page("dialogue")
    for _ in range(3):
        check("repair capability selected", ui.execute_action("agentTypeNext"))
    check("second repair instance selected", ui.execute_action("agentInstanceNext"))
    check("specific repair card", ui.execute_action("agentCollectCard"))
    check("repair card names second instance", second_row["id"][:8] in ai.get_candidate_text())
    check("repair card quotes selected instance", f"所选实例当前耐久：{second_worn:.0f}" in ai.get_candidate_text()
          and "预计消耗（结算前复核）" in ai.get_candidate_text())
    check("specific repair confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    axes = {guid_key(guid(row["id"])): row["durability"] for row in inventory_instances("axe")}
    check("only selected instance repaired", axes[guid_key(first_axe)] == 60 and axes[guid_key(second_axe)] == 80)
    repair_point = save_new_point("repair save")
    check("repair reload", save.load_point(repair_point))
    axes = {guid_key(guid(row["id"])): row["durability"] for row in inventory_instances("axe")}
    check("repair reload preserves target and durability", axes[guid_key(first_axe)] == 60 and axes[guid_key(second_axe)] == 80)
    ui.open_page("hud")
    camp = subsystem(unreal.HearthwardCampSubsystem, world)
    check("pen rescue prerequisite", camp.record_rescue("task050_pen_fixture"))
    check("pen tier prerequisite", camp.upgrade_camp(store.get_timeline_epoch()))
    player.set_actor_location(unreal.Vector(-2300, -2200, 100), False, True)
    pc.set_control_rotation(unreal.Rotator(pitch=-15, yaw=0))
    yield delay(1)
    ui.open_nature(unreal.Guid())
    check("build pen through UI", ui.execute_action("nature.build_pen:goat"))
    yield wait(lambda: not nature.busy(), 10)
    check("pen exists", len(nature_state()["pens"]) == 1)
    pen = nature_state()["pens"][0]
    pen_pos = pen["position"]
    player.set_actor_location(unreal.Vector(pen_pos["x"] - 100, pen_pos["y"], pen_pos["z"] + 100), False, True)
    brother.set_actor_location(unreal.Vector(pen_pos["x"] - 180, pen_pos["y"], pen_pos["z"] + 100), False, True)
    check("brother feed fixture", brother.bag.try_add("feed", 10) == unreal.HearthwardInventoryResult.SUCCESS)
    ui.open_nature(guid(pen["id"]))
    check("brother feed UI card", ui.execute_action("nature.brother_deposit_feed:"))
    check("brother feed confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    check("pen paid once from brother bag", nature_state()["pens"][0]["feed"] == 10
          and brother.bag.get_item_count("feed") == 0)

    # A real rescue interrupts a second consumptive task. Completion waits for a new player command.
    check("second feed fixture", brother.bag.try_add("feed", 10) == unreal.HearthwardInventoryResult.SUCCESS)
    ui.open_nature(guid(pen["id"]))
    check("rescue test feed card", ui.execute_action("nature.brother_deposit_feed:"))
    check("rescue test feed confirmed", ai.confirm_candidate(ai.get_candidate_id()))
    game.apply_damage(10000)
    yield wait(lambda: game.health > 0, 12)
    check("rescue leaves production paused", brother.get_phase() == unreal.HearthwardCompanionPhase.HOLDING_SAFELY
          and nature_state()["pens"][0]["feed"] == 10 and brother.bag.get_item_count("feed") == 10)
    check("explicit resume after rescue", brother.resume_blocked(player))
    yield wait(lambda: brother.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 25)
    check("resumed feed settles once", nature_state()["pens"][0]["feed"] == 20
          and brother.bag.get_item_count("feed") == 0)

    # Return together. Skipped days grant one reminder opportunity, not one per day.
    player.set_actor_location(unreal.Vector(-150, -150, 100), False, True)
    brother.set_actor_location(unreal.Vector(-200, 100, 100), False, True)
    ui.open_page("hud")
    check("camp conversation clears old notices", ai.query_recent_history(player, brother))
    player_food = max(0, 30 - bag.get_item_count("wild_food"))
    brother_food = max(0, 20 - brother.bag.get_item_count("wild_food"))
    check("player food fixture", not player_food or bag.try_add("wild_food", player_food) == unreal.HearthwardInventoryResult.SUCCESS)
    check("brother food fixture", not brother_food or brother.bag.try_add("wild_food", brother_food) == unreal.HearthwardInventoryResult.SUCCESS)
    for index in range(9):
        check("W advance " + str(index), clock.advance_calendar(480) > 479)
        for _ in range(3):
            game.use_item("wild_food")
        yield delay(.1)
    yield delay(1)
    report["initiative_kind"] = str(ai.get_initiative_kind())
    check("three-day reminder once", ai.has_active_initiative() and str(ai.get_initiative_kind()) == "conversation_reminder")
    check("reminder is a short line", "哥" in ai.get_initiative_line())
    yield delay(.5)
    reminder_point = save_new_point("reminder save")
    check("reminder reload", save.load_point(reminder_point))
    ui.open_page("hud")
    unreal.GameplayStatics.set_game_paused(world, False)
    yield delay(.5)
    check("same visit does not repeat after reload", not ai.has_active_initiative())
    player.set_actor_location(unreal.Vector(5000, 5000, 100), False, True)
    yield delay(.5)
    player.set_actor_location(unreal.Vector(-150, -150, 100), False, True)
    yield wait(lambda: ai.has_active_initiative(), 8)
    check("new camp visit permits one reminder", str(ai.get_initiative_kind()) == "conversation_reminder")

    # One confirmation cancels the active task and listed rules; player claims and cargo survive.
    check("clear reminder before reset", ai.query_recent_history(player, brother))
    check("reset claim fixture", ai.put_player_memory(player, brother, unreal.Guid(), "claim", "我说营地有很多木材", "wood"))
    check("reset agreement fixture", ai.put_player_memory(player, brother, unreal.Guid(), "agreement", "以后少采木材", "wood"))
    check("reset restriction fixture", ai.put_player_memory(player, brother, unreal.Guid(), "collection_ban", "不要采木材", "wood"))
    reset_wood = brother.bag.get_item_count("wood")
    check("reset has cargo", reset_wood > 0)
    ui.open_page("dialogue")
    check("reset active task card", ai.set_structured_goal(player, brother, goal("store", "wood", 1, "held_to_camp", "bag")))
    check("reset active task confirmation", ai.confirm_candidate(ai.get_candidate_id()))
    ui.open_page("memory")
    check("reset review card", ui.execute_action("memoryReset"))
    check("reset confirmation", ui.execute_action("confirm"))
    check("reset stops task and preserves cargo", brother.get_phase() == unreal.HearthwardCompanionPhase.CANCELLED
          and brother.bag.get_item_count("wood") == reset_wood)
    kinds = [str(row.kind) for row in ai.get_player_memories()]
    check("reset preserves claim only", "claim" in kinds and "agreement" not in kinds
          and "collection_ban" not in kinds and "typed_constraint" not in kinds)
    ui.open_page("hud")
    reset_point = save_new_point("reset save")
    check("reset reload", save.load_point(reset_point))
    kinds = [str(row.kind) for row in ai.get_player_memories()]
    check("reset reload preserves claim and revoked rules", "claim" in kinds and "agreement" not in kinds
          and "collection_ban" not in kinds and brother.bag.get_item_count("wood") == reset_wood)
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
                    raise TimeoutError("phase=" + str(st.get("brother").get_phase()) + " block=" + str(st.get("brother").block_reason))
                return
        pending = next(flow)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
