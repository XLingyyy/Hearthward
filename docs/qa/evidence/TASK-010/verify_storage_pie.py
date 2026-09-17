"""Isolated runtime actors exercise shared camp access; no content assets are modified."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task010"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
R = unreal.HearthwardInventoryResult
report = {"ok": False, "sessions": []}
st = {"phase": "start", "round": 0, "deadline": time.monotonic() + 150}

def phase(name):
    st.update(phase=name, since=time.monotonic(), frames=0)

def check(name, value):
    st["entry"]["checks"][name] = bool(value)

def guid():
    return unreal.GuidLibrary.new_guid()

def transfer(point, to_camp, count, op=None, epoch=None, item="wood"):
    return point.transfer(st["inv"], to_camp, item, count, op or guid(), epoch or st["storage"].get_timeline_epoch())

def inventory_changed():
    st["entry"]["inventory_events"] += 1
    if st.get("reentry"):
        st["reentry"] = False
        check("event_sees_both_ends_committed", st["inv"].get_item_count("wood") == 0 and st["b"].get_item_count("wood") == 100)
        reply = transfer(st["b"], True, 100, st["first_op"])
        check("reentrant_duplicate_not_applied", reply.result == R.SUCCESS and reply.replayed and reply.moved_count == 0)

def transferred(op, to_camp, item, count):
    st["entry"]["transfer_events"].append({"to_camp": to_camp, "item": str(item), "count": count})

def shot(label):
    if st["round"] != 1: return
    unreal.SystemLibrary.execute_console_command(st["world"], f'HighResShot 1280x720 filename="{(out / (label + ".png")).as_posix()}"', st["pc"])

def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and len(report["sessions"]) == 2 and all(all(e["checks"].values()) for e in report["sessions"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

def tick(dt):
    try:
        if time.monotonic() > st["deadline"]: raise TimeoutError("Storage PIE timed out")
        p = st["phase"]
        st["frames"] = st.get("frames", 0) + 1
        wait = time.monotonic() - st.get("since", 0) if st["frames"] > 2 else 0
        if p == "start":
            st["round"] += 1
            levels.editor_request_begin_play()
            phase("possess")
        elif p == "possess":
            if not levels.is_in_play_in_editor(): return
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            if not pawn or not pc or not pc.get_hud(): return
            actions = [a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer() == pawn]
            bindings = [(s, a) for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                        if isinstance(s.get_outer(), unreal.LocalPlayer) for a in actions if len(s.query_keys_mapped_to_action(a)) == 4]
            if not bindings: return
            sub, move = bindings[0]
            storage = next(c for c in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if c.get_outer() == world)
            unreal.SystemLibrary.execute_console_command(world, "Hearthward.Storage.CreateTestAccess", pc)
            actors = unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Storage.TestAccess")
            assert len(actors) == 2
            points = [a.get_component_by_class(unreal.HearthwardStorageAccessComponent) for a in actors]
            e = {"round": st["round"], "checks": {}, "inventory_events": 0, "transfer_events": []}
            report["sessions"].append(e)
            st.update(world=world, pawn=pawn, pc=pc, hud=pc.get_hud(), inv=pawn.get_component_by_class(unreal.HearthwardInventoryComponent),
                      storage=storage, a=points[0], b=points[1], actors=actors, sub=sub, move=move, entry=e)
            inv, a, b = st["inv"], st["a"], st["b"]
            inv.on_inventory_changed.add_callable(inventory_changed)
            storage.on_transferred.add_callable(transferred)
            check("fresh_world_empty", inv.get_weight() == 0 and a.get_item_count("wood") == b.get_item_count("wood") == 0)
            ids = [a.get_container_id(), b.get_container_id(), storage.get_container_id()]
            e["container_ids"] = [v.to_string() for v in ids]
            e["actor_paths"] = [actor.get_path_name() for actor in actors]
            check("two_actor_accesses_one_container", actors[0] != actors[1] and unreal.GuidLibrary.equal_equal_guid_guid(ids[0], ids[1]) and unreal.GuidLibrary.equal_equal_guid_guid(ids[1], ids[2]))
            assert inv.try_add("wood", 100) == R.SUCCESS
            st.update(first_op=guid(), reentry=True)
            reply = transfer(a, True, 100, st["first_op"])
            check("deposit_actual_delta", reply.result == R.SUCCESS and reply.moved_count == 100 and not reply.replayed)
            check("deposit_restores_carry_speed", pawn.get_movement_component().max_walk_speed == 350)
            assert inv.try_add("wood", 20) == R.SUCCESS
            assert transfer(b, True, 20).result == R.SUCCESS
            check("shared_capacity_exceeds_bag", a.get_item_count("wood") == b.get_item_count("wood") == 120 and storage.get_weight() == 120)
            reply = transfer(b, False, 50)
            check("other_access_withdraws", reply.moved_count == 50 and a.get_item_count("wood") == 70 and inv.get_item_count("wood") == 50)
            check("weight_and_speed_follow_transfer", inv.get_weight() == 50 and pawn.get_movement_component().max_walk_speed == 332.5)
            st["hud"].toggle_inventory()
            phase("half_panel")
        elif p == "half_panel" and wait > 0.3:
            shot("withdrawn-half")
            phase("after_half")
        elif p == "after_half" and wait > 0.3:
            inv, a, b, storage = st["inv"], st["a"], st["b"], st["storage"]
            assert transfer(a, False, 50).moved_count == 50
            check("full_after_withdrawal", inv.get_weight() == 100 and a.get_item_count("wood") == 20)
            before = st["entry"]["inventory_events"], len(st["entry"]["transfer_events"])
            failed = guid()
            reply = transfer(b, False, 1, failed)
            check("overweight_rejected", reply.result == R.CAPACITY_EXCEEDED and reply.moved_count == 0)
            check("insufficient_rejected", transfer(a, True, 101).result == R.INSUFFICIENT_ITEMS)
            check("payload_conflict_rejected", transfer(b, False, 100, st["first_op"]).result == R.OPERATION_CONFLICT)
            check("invalid_component_rejected", a.transfer(None, True, "wood", 1, guid(), storage.get_timeline_epoch()).result == R.INVALID_ARGUMENT)
            check("failure_no_mutation_or_event", inv.get_weight() == 100 and a.get_item_count("wood") == 20 and before == (st["entry"]["inventory_events"], len(st["entry"]["transfer_events"])))
            st["failed"] = failed
            shot("withdrawn-full")
            phase("after_full")
        elif p == "after_full" and wait > 0.3:
            inv, a, b, storage = st["inv"], st["a"], st["b"], st["storage"]
            assert transfer(a, True, 10).moved_count == 10
            reply = transfer(b, False, 1, st["failed"])
            check("failed_retry_stable_after_capacity_freed", reply.replayed and reply.result == R.CAPACITY_EXCEEDED and inv.get_item_count("wood") == 90)
            old_epoch = storage.get_timeline_epoch()
            storage.advance_timeline()
            reply = transfer(b, True, 100, st["first_op"], old_epoch)
            check("old_epoch_rejected", reply.result == R.STALE_TIMELINE and reply.moved_count == 0)
            check("epoch_does_not_erase_inventory", inv.get_item_count("wood") == 90 and a.get_item_count("wood") == 30)
            assert transfer(b, False, 10).moved_count == 10
            check("quantity_conserved", inv.get_item_count("wood") + a.get_item_count("wood") == 120)
            check("only_success_emits_events", len(st["entry"]["transfer_events"]) == 6 and st["entry"]["inventory_events"] == 8)
            st["hud"].toggle_inventory()
            phase("move")
        elif p == "move":
            st["sub"].inject_input_vector_for_action(st["move"], unreal.Vector(0, 1, 0), [], [])
            if wait < 0.8: return
            speed = st["pawn"].get_velocity().length()
            st["entry"]["full_load_actual_speed"] = speed
            check("actual_full_load_movement", abs(speed - 315) < 0.5)
            levels.editor_request_end_play()
            phase("stopped")
        elif p == "stopped" and not levels.is_in_play_in_editor() and wait > 1:
            if st["round"] == 2: finish()
            else: phase("start")
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
