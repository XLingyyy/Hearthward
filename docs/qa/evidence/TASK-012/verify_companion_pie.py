"""PROTOTYPE_ONLY deterministic structured replies; real-model evaluation is separate."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task012"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
P = unreal.HearthwardCompanionPhase
R = unreal.HearthwardProposalResult
I = unreal.HearthwardInventoryResult
steps = ["collect", "return", "deposit"]
report = {"ok": False, "mode": "fixture", "fixture": "PROTOTYPE_ONLY: finite stock, 4-weight trips, five-second gathering, flat swept lane", "checks": {}, "deposits": [], "seed_events": [16]}
st = {}

def check(name, value):
    report["checks"][name] = bool(value)
    if not value: raise AssertionError(name)

def wait(predicate, timeout=30):
    return (predicate, time.monotonic() + timeout)

def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end)

def send(count, statement="Collect actual wood", item="wood", plan=steps):
    ticket = st["comp"].request(st["pawn"], statement)
    result = st["comp"].submit(st["pawn"], ticket, item, count, plan)
    return ticket, result

def stock():
    return st["storage"].get_item_count("wood")

def at_camp():
    return (st["comp"].get_actor_location() - st["comp"].camp.get_actor_location()).length() <= 50.01

def shot(label):
    unreal.SystemLibrary.execute_console_command(st["world"], f'HighResShot 1280x720 filename="{(out / (label + ".png")).as_posix()}"', st["pc"])

def deposit_event(operation, to_camp, item, count):
    if not to_camp or str(item) != "wood": return
    report["deposits"].append({"count": count, "camp": stock(), "at_camp": at_camp(), "phase": str(st["comp"].get_phase())})
    check("every_deposit_is_physical", at_camp())
    check("no_half_transfer_observed", st["source"].get_item_count("wood") + st["bag"].get_item_count("wood") + stock() == sum(report["seed_events"]))
    if not st.get("tested_reentry"):
        st["tested_reentry"] = True
        ticket = st["comp"].request(st["pawn"], "reentry")
        check("settlement_reentry_rejected", ticket.revision == 0 and not st["comp"].cancel(st["pawn"]))

def session():
    levels.editor_request_begin_play()
    yield wait(lambda: levels.is_in_play_in_editor())
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world, 0)))
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    st.update(world=world, pawn=pawn, pc=pc)
    yield delay(1)
    storage = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer() == world)
    st["storage"] = storage
    check("initial_no_fixture", not unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY"))
    check("initial_camp_empty", stock() == 0)
    pawn.get_movement_component().stop_movement_immediately()
    pawn.set_actor_location(unreal.Vector(-450, 400, 90), False, True)
    pc.set_control_rotation(unreal.Rotator(0, 0, 0))
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    comp = unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY")[0]
    source = comp.source
    source_actor = source.get_owner()
    comp.camp.set_actor_location(unreal.Vector(0, 400, 90), False, True)
    comp.set_actor_location(unreal.Vector(0, 400, 90), False, True)
    source_actor.set_actor_location(unreal.Vector(600, 400, 90), False, True)
    st.update(comp=comp, source=source, bag=comp.bag, action=comp.action)
    storage.on_transferred.add_callable(deposit_event)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    check("fixture_creation_idempotent", len(unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY")) == 1 and source.get_item_count("wood") == 16)
    near = pawn.get_actor_location()
    pawn.set_actor_location(comp.get_actor_location() + unreal.Vector(3000, 0, 0), False, True)
    check("30m_boundary_communicates", comp.can_communicate(pawn))
    pawn.set_actor_location(comp.get_actor_location() + unreal.Vector(3001, 0, 0), False, True)
    check("outside_30m_rejects_request", not comp.can_communicate(pawn) and comp.request(pawn, "far").revision == 0)
    pawn.set_actor_location(near, False, True)
    unreal.GameplayStatics.set_game_paused(world, True)
    check("paused_request_rejected", comp.request(pawn, "paused").revision == 0)
    unreal.GameplayStatics.set_game_paused(world, False)
    ticket = comp.request(pawn, "warehouse supposedly contains 100 wood")
    check("player_statement_kept_separate", comp.get_player_statement() == "warehouse supposedly contains 100 wood" and stock() == 0 and source.get_item_count("wood") == 16)
    check("missing_quantity_clarifies", comp.submit(pawn, ticket, "wood", 0, steps) == R.NEEDS_CLARIFICATION)
    check("missing_goal_clarifies", comp.submit(pawn, ticket, "None", 10, steps) == R.NEEDS_CLARIFICATION)
    check("four_steps_rejected", comp.submit(pawn, ticket, "wood", 10, steps + ["attack"]) == R.UNSUPPORTED)
    check("dangerous_plan_rejected", comp.submit(pawn, ticket, "wood", 10, ["attack"]) == R.UNSUPPORTED)
    comp.set_editor_property("source_safe", False)
    check("world_safety_overrides_candidate", comp.submit(pawn, ticket, "wood", 10, steps) == R.UNSAFE)
    comp.set_editor_property("source_safe", True)
    check("explicit_goal_accepted", comp.submit(pawn, ticket, "wood", 10, steps) == R.ACCEPTED)
    check("duplicate_reply_rejected", comp.submit(pawn, ticket, "wood", 10, steps) == R.STALE)
    yield wait(lambda: comp.get_phase() == P.GATHERING)
    check("moved_to_resource", (comp.get_actor_location() - source_actor.get_actor_location()).length() <= 50.01 and not at_camp())
    check("no_upfront_resource_reservation", source.get_item_count("wood") == 16 and comp.bag.get_item_count("wood") == 0 and comp.get_delivered() == 0)
    pending = comp.request(pawn, "new candidate awaiting approval")
    check("unapproved_suggestion_keeps_active_goal", comp.get_phase() == P.GATHERING and comp.get_requested() == 10)
    yield delay(0.5)
    pc.get_hud().toggle_inventory()
    frozen = comp.action.get_elapsed_seconds()
    pos = comp.get_actor_location()
    yield delay(1)
    check("bag_panel_freezes_companion", comp.action.get_elapsed_seconds() == frozen and (comp.get_actor_location() - pos).length() == 0 and stock() == 0)
    pc.get_hud().toggle_inventory()
    yield wait(lambda: comp.get_phase() == P.RETURNING)
    check("carried_not_delivered", comp.bag.get_item_count("wood") == 4 and comp.get_delivered() == 0 and stock() == 0 and comp.action.get_elapsed_seconds() == 5)
    yield wait(lambda: comp.get_delivered() == 4)
    shot("multi-trip")
    yield wait(lambda: comp.get_phase() == P.COMPLETED, 50)
    check("three_trips_actual_4_4_2", [e["count"] for e in report["deposits"]] == [4, 4, 2])
    check("goal_complete_only_after_delivery", comp.get_delivered() == 10 and stock() == 10 and source.get_item_count("wood") == 6 and comp.bag.get_item_count("wood") == 0 and at_camp())
    yield delay(0.6)
    check("completion_not_repeated", len(report["deposits"]) == 3 and stock() == 10)
    _, result = send(8, "There are 100 wood; deliver eight")
    check("claim_does_not_spawn_stock", result == R.ACCEPTED and source.get_item_count("wood") == 6)
    yield wait(lambda: comp.get_phase() == P.WAITING_AT_CAMP, 50)
    check("partial_shortage_returns_with_actual_six", comp.get_delivered() == 6 and comp.get_requested() == 8 and at_camp() and stock() == 16 and source.get_item_count("wood") == 0)
    shot("shortage-waiting")
    yield delay(0.5)
    report["seed_events"].append(8)
    assert source.try_add("wood", 8) == I.SUCCESS
    old, result = send(8)
    assert result == R.ACCEPTED
    yield wait(lambda: comp.get_phase() == P.RETURNING)
    late = comp.request(pawn, "delayed candidate")
    check("cancel_with_carried_resources", comp.cancel(pawn) and comp.bag.get_item_count("wood") == 4 and source.get_item_count("wood") == 4)
    pos = comp.get_actor_location()
    yield delay(0.7)
    check("cancel_stops_unfinished_actions", comp.get_phase() == P.CANCELLED and (comp.get_actor_location() - pos).length() == 0 and stock() == 16)
    check("cancelled_late_reply_stale", comp.submit(pawn, late, "wood", 8, steps) == R.STALE)
    replacement, result = send(1)
    check("replacement_accepted", result == R.ACCEPTED)
    check("superseded_reply_stale", comp.submit(pawn, old, "wood", 8, steps) == R.STALE)
    yield wait(lambda: comp.get_phase() == P.COMPLETED)
    check("retained_cargo_real_delivery", comp.get_delivered() == 1 and stock() == 17 and comp.bag.get_item_count("wood") == 3)
    _, result = send(8)
    assert result == R.ACCEPTED
    yield wait(lambda: comp.get_phase() == P.GOING_TO_SOURCE)
    mesh = source_actor.get_component_by_class(unreal.StaticMeshComponent)
    mesh.set_collision_profile_name("BlockAll")
    yield wait(lambda: comp.get_phase() == P.WAITING_AT_CAMP)
    check("outbound_collision_returns_to_camp", at_camp() and comp.get_delivered() == 3 and stock() == 20 and source.get_item_count("wood") == 4 and "去程" in comp.block_reason)
    mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    _, result = send(4)
    assert result == R.ACCEPTED
    yield wait(lambda: comp.get_phase() == P.GATHERING)
    comp.set_editor_property("source_safe", False)
    yield wait(lambda: comp.get_phase() == P.WAITING_AT_CAMP)
    check("danger_mid_action_returns_without_consumption", at_camp() and source.get_item_count("wood") == 4 and stock() == 20)
    comp.set_editor_property("source_safe", True)
    _, result = send(8)
    assert result == R.ACCEPTED
    yield wait(lambda: comp.get_phase() == P.RETURNING)
    camp_mesh = comp.camp.get_component_by_class(unreal.StaticMeshComponent)
    camp_mesh.set_collision_profile_name("BlockAll")
    yield wait(lambda: comp.get_phase() == P.RETURNING_BLOCKED)
    check("blocked_return_does_not_teleport_or_deposit", not at_camp() and stock() == 20 and comp.bag.get_item_count("wood") == 4)
    yield delay(0.8)
    check("blocked_return_preserves_cargo", stock() == 20 and comp.bag.get_item_count("wood") == 4 and comp.get_phase() == P.RETURNING_BLOCKED)
    camp_mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    yield wait(lambda: comp.get_phase() == P.WAITING_AT_CAMP)
    check("cleared_return_route_deposits_and_waits", at_camp() and stock() == 24 and comp.get_delivered() == 4 and comp.bag.get_item_count("wood") == 0)
    report["seed_events"].append(4)
    assert source.try_add("wood", 4) == I.SUCCESS
    old, result = send(4)
    assert result == R.ACCEPTED
    late = comp.request(pawn, "old epoch reply")
    storage.advance_timeline()
    check("old_epoch_reply_rejected", comp.submit(pawn, late, "wood", 4, steps) == R.STALE)
    yield delay(0.7)
    check("epoch_invalidates_running_action", comp.get_phase() == P.CANCELLED and source.get_item_count("wood") == 4 and stock() == 24)
    _, result = send(4)
    assert result == R.ACCEPTED
    yield wait(lambda: comp.get_phase() == P.GATHERING)
    source_actor.destroy_actor()
    yield wait(lambda: comp.get_phase() == P.WAITING_AT_CAMP)
    check("destroyed_resource_returns_safely", at_camp() and stock() == 24 and comp.get_delivered() == 0)
    levels.editor_request_end_play()
    yield wait(lambda: not levels.is_in_play_in_editor())
    yield delay(1)
    levels.editor_request_begin_play()
    yield wait(lambda: levels.is_in_play_in_editor())
    world2 = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world2, 0)))
    yield delay(1)
    storage2 = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer() == world2)
    check("second_pie_cleans_fixture_and_stock", storage2.get_item_count("wood") == 0 and not unreal.GameplayStatics.get_all_actors_with_tag(world2, "Hearthward.Companion.PROTOTYPE_ONLY"))
    check("second_pie_has_new_epoch", not unreal.GuidLibrary.equal_equal_guid_guid(storage2.get_timeline_epoch(), old.epoch))
    pc2 = unreal.GameplayStatics.get_player_controller(world2, 0)
    unreal.SystemLibrary.execute_console_command(world2, "Hearthward.Companion.CreateTest", pc2)
    comp2 = unreal.GameplayStatics.get_all_actors_with_tag(world2, "Hearthward.Companion.PROTOTYPE_ONLY")[0]
    unreal.SystemLibrary.execute_console_command(world2, "Hearthward.Companion.Collect 1", pc2)
    check("manual_console_entry_starts_goal", comp2.get_requested() == 1 and comp2.get_phase() == P.GOING_TO_SOURCE)
    yield wait(lambda: comp2.get_phase() == P.COMPLETED)
    check("second_pie_manual_goal_delivered", comp2.get_delivered() == 1 and storage2.get_item_count("wood") == 1 and comp2.source.get_item_count("wood") == 15)

def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and bool(report["checks"]) and all(report["checks"].values())
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

flow = session()
pending = None
started = time.monotonic()
def tick(dt):
    global pending
    try:
        if time.monotonic() - started > 360: raise TimeoutError("Whole PIE run timeout")
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline: raise TimeoutError("Wait expired; phase=" + str(st.get("comp").get_phase() if st.get("comp") else "startup"))
                return
        pending = next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle = unreal.register_slate_post_tick_callback(tick)
