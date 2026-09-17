"""Exercise native inventory and movement twice, without changing assets."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task008"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "sessions": []}
st = {"phase": "start", "round": 0, "deadline": time.monotonic() + 150}
R = unreal.HearthwardInventoryResult

def changed():
    st["entry"]["change_events"] += 1

def phase(name):
    st.update(phase=name, since=time.monotonic())

def inject(y=0):
    st["sub"].inject_input_vector_for_action(st["move"], unreal.Vector(0, y, 0), [], [])

def command(text):
    unreal.SystemLibrary.execute_console_command(st["world"], text, st["pc"])

def shot(label):
    command(f'HighResShot 1280x720 filename="{(out / (str(st["round"]) + "-" + label)).as_posix()}.png"')

def begin_move(label, speed):
    st["pawn"].get_movement_component().stop_movement_immediately()
    st["pawn"].set_actor_location(st["home"], False, True)
    st.update(label=label, expected_speed=speed)
    phase("moving")

def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and len(report["sessions"]) == 2 and all(all(e["checks"].values()) for e in report["sessions"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

def tick(dt):
    try:
        if time.monotonic() > st["deadline"]: raise TimeoutError("Inventory PIE timed out")
        p = st["phase"]
        wait = time.monotonic() - st.get("since", 0)
        if p == "start":
            st["round"] += 1
            levels.editor_request_begin_play()
            phase("possess")
        elif p == "possess":
            if not levels.is_in_play_in_editor(): return
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not pawn: return
            inv = pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
            actions = sorted((a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer() == pawn), key=lambda a: a.get_name())
            bindings = [(subsystem, action) for subsystem in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                        if isinstance(subsystem.get_outer(), unreal.LocalPlayer)
                        for action in actions if len(subsystem.query_keys_mapped_to_action(action)) == 4]
            # Pawn creation precedes deferred Enhanced Input mapping rebuild.
            if not bindings: return
            sub, move = bindings[0]
            e = {"round": st["round"], "change_events": 0, "speeds": {}, "checks": {}}
            report["sessions"].append(e)
            st.update(world=world, pawn=pawn, pc=unreal.GameplayStatics.get_player_controller(world, 0), inv=inv,
                      sub=sub, move=move, home=pawn.get_actor_location(), entry=e,
                      action=pawn.get_component_by_class(unreal.HearthwardTimedActionComponent))
            inv.on_inventory_changed.add_callable(changed)
            e["checks"]["fresh_empty"] = inv.get_weight() == 0 and inv.get_capacity() == 100
            phase("settle")
        elif p == "settle" and wait > 0.4:
            begin_move("empty", 350)
        elif p == "moving":
            inject(1)
            if wait < 0.8: return
            e = st["entry"]
            actual = st["pawn"].get_velocity().length()
            e["speeds"][st["label"]] = actual
            e["checks"][st["label"] + "_actual_speed"] = abs(actual - st["expected_speed"]) < 0.5
            inject()
            st["pawn"].get_movement_component().stop_movement_immediately()
            if st["label"] == "unloaded":
                e["checks"]["loaded_action_movement_interrupt"] = st["action"].get_status() == unreal.HearthwardTimedActionStatus.INTERRUPTED
            shot(st["label"])
            phase("after_move")
        elif p == "after_move" and wait > 0.35:
            inv, e = st["inv"], st["entry"]
            label = st["label"]
            if label == "empty":
                command("Hearthward.Inventory.Add wood 50")
                e["checks"]["console_adds_real_items"] = inv.get_item_count("wood") == 50 and inv.get_weight() == 50
                before = e["change_events"]
                e["checks"]["excess_rejected"] = inv.try_add("ore", 26) == R.CAPACITY_EXCEEDED
                e["checks"]["insufficient_rejected"] = inv.try_remove("wood", 51) == R.INSUFFICIENT_ITEMS
                e["checks"]["unknown_rejected"] = inv.try_add("missing", 1) == R.UNKNOWN_ITEM
                e["checks"]["negative_rejected"] = inv.try_remove("wood", -1) == R.INVALID_COUNT
                command("Hearthward.Inventory.Add wood 1.5")
                command("Hearthward.Inventory.Add wood 999999999999999999999")
                e["checks"]["failure_has_no_side_effect_or_event"] = before == e["change_events"] == 1 and inv.get_weight() == 50 and inv.get_item_count("wood") == 50
                e["checks"]["half_stamina_multiplier"] = abs(inv.get_stamina_cost_multiplier() - 1.05) < 0.00001
                begin_move("half", 332.5)
            elif label == "half":
                assert inv.try_add("ore", 25) == R.SUCCESS
                e["checks"]["mixed_exact_capacity"] = inv.get_weight() == 100 and inv.get_item_count("ore") == 25
                e["checks"]["full_rejects_one_arrow"] = inv.try_add("arrow", 1) == R.CAPACITY_EXCEEDED and inv.get_item_count("arrow") == 0 and e["change_events"] == 2
                begin_move("full", 315)
            elif label == "full":
                assert st["action"].start_action()
                phase("action_running")
            elif label == "unloaded":
                assert inv.try_add("arrow", 2000) == R.SUCCESS
                assert inv.try_remove("arrow", 1) == R.SUCCESS
                e["checks"]["fractional_weight_exact"] = inv.get_weight() == 99.95 and inv.get_item_count("arrow") == 1999
                e["checks"]["fractional_capacity_rejected"] = inv.try_add("meat", 1) == R.CAPACITY_EXCEEDED and inv.get_item_count("meat") == 0
                shot("arrows")
                phase("cleanup")
        elif p == "action_running" and wait > 1:
            st["entry"]["checks"]["action_runs_at_full_load"] = st["action"].get_status() == unreal.HearthwardTimedActionStatus.RUNNING and st["action"].get_elapsed_seconds() > 0
            shot("full-action")
            phase("unload")
        elif p == "unload" and wait > 0.3:
            inv = st["inv"]
            command("Hearthward.Inventory.Remove wood 50")
            assert inv.try_remove("ore", 25) == R.SUCCESS
            st["entry"]["checks"]["removal_does_not_cancel_action"] = st["action"].get_status() == unreal.HearthwardTimedActionStatus.RUNNING
            st["entry"]["checks"]["unloaded_weight"] = inv.get_weight() == 0
            begin_move("unloaded", 350)
        elif p == "cleanup" and wait > 0.3:
            assert st["inv"].try_remove("arrow", 1999) == R.SUCCESS
            st["entry"]["checks"]["one_event_per_success"] = st["entry"]["change_events"] == 7
            st["entry"]["checks"]["final_empty"] = st["inv"].get_weight() == 0 and st["inv"].get_item_count("arrow") == 0
            levels.editor_request_end_play()
            phase("stopped")
        elif p == "stopped" and not levels.is_in_play_in_editor() and wait > 1:
            if st["round"] == 2: finish()
            else: phase("start")
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
