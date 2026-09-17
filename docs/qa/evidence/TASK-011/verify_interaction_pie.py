"""PROTOTYPE_ONLY: 200cm target and 2-wood settlement fixture; no final recipe/building."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task011"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
S = unreal.HearthwardInteractionStatus
R = unreal.HearthwardInventoryResult
report = {"ok": False, "fixture": "PROTOTYPE_ONLY: 200cm, 2 wood; script-only consumer, no building", "sessions": []}
st = {"phase": "start", "round": 0, "deadline": time.monotonic() + 240}

def phase(name):
    st.update(phase=name, since=time.monotonic(), frames=0)

def check(name, value):
    st["entry"]["checks"][name] = bool(value)

def ready(actor):
    e = st["entry"]
    e["ready_callbacks"] += 1
    e["completion_elapsed"].append(st["action"].get_elapsed_seconds())
    result = st["inv"].try_remove("wood", 2)
    e["settlement_results"].append(str(result))
    if result == R.SUCCESS: e["settled"] += 1

def create_target():
    unreal.SystemLibrary.execute_console_command(st["world"], "Hearthward.Interaction.CreateTestTarget", st["pc"])
    actors = unreal.GameplayStatics.get_all_actors_with_tag(st["world"], "Hearthward.Interaction.PROTOTYPE_ONLY")
    assert len(actors) == 1
    st["target_actor"] = actors[0]
    st["target"] = actors[0].get_component_by_class(unreal.HearthwardInteractionTargetComponent)
    st["target"].on_interaction_ready.add_callable(ready)

def place(distance):
    st["target_actor"].set_actor_location(st["pawn"].get_actor_location() + unreal.Vector(distance, 0, 0), False, True)

def begin():
    st["pawn"].get_movement_component().stop_movement_immediately()
    assert st["interaction"].begin_interaction(st["target"])

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
        if time.monotonic() > st["deadline"]: raise TimeoutError("Interaction PIE timed out")
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
                        if isinstance(s.get_outer(), unreal.LocalPlayer) for a in actions if a.get_name() == "InteractAction" and len(s.query_keys_mapped_to_action(a)) == 1]
            if not bindings: return
            sub, interact = bindings[0]
            move = next(a for a in actions if len(sub.query_keys_mapped_to_action(a)) == 4)
            e = {"round": st["round"], "checks": {}, "ready_callbacks": 0, "settled": 0, "completion_elapsed": [], "settlement_results": []}
            report["sessions"].append(e)
            st.update(world=world, pawn=pawn, pc=pc, hud=pc.get_hud(), entry=e, sub=sub, interact=interact, move=move,
                      inv=pawn.get_component_by_class(unreal.HearthwardInventoryComponent),
                      action=pawn.get_component_by_class(unreal.HearthwardTimedActionComponent),
                      interaction=pawn.get_component_by_class(unreal.HearthwardInteractionComponent),
                      clock=next(c for c in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if c.get_outer() == world))
            check("fresh_session", st["inv"].get_weight() == 0 and st["interaction"].get_status() == S.IDLE and len(unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Interaction.PROTOTYPE_ONLY")) == 0)
            check("e_mapping", sub.query_keys_mapped_to_action(interact)[0].export_text() == "E")
            phase("settle")
        elif p == "settle" and wait > 0.6:
            create_target()
            inv, interaction = st["inv"], st["interaction"]
            assert inv.try_add("wood", 10) == R.SUCCESS
            check("null_target_rejected", not interaction.begin_interaction(None) and interaction.get_status() == S.INVALID_TARGET)
            st["target"].set_editor_property("max_distance", 0)
            check("unconfigured_rejected", not interaction.begin_interaction(st["target"]) and interaction.get_status() == S.UNCONFIGURED)
            st["target"].set_editor_property("max_distance", 200)
            place(201)
            check("outside_range_rejected", not interaction.begin_interaction(st["target"]) and interaction.get_status() == S.OUT_OF_RANGE)
            place(200)
            begin()
            check("boundary_accepted", interaction.get_status() == S.RUNNING)
            check("duplicate_start_preserves_active", not interaction.begin_interaction(st["target"]) and interaction.get_status() == S.RUNNING)
            phase("before_move")
        elif p == "before_move" and wait > 0.5:
            check("no_upfront_charge", st["inv"].get_item_count("wood") == 10 and st["entry"]["ready_callbacks"] == 0)
            st["sub"].inject_input_vector_for_action(st["move"], unreal.Vector(0, 1, 0), [], [])
            phase("move_interrupted")
        elif p == "move_interrupted" and wait > 0.4:
            check("movement_interrupts_without_charge", st["interaction"].get_status() == S.INTERRUPTED and st["inv"].get_item_count("wood") == 10)
            st["pawn"].get_movement_component().stop_movement_immediately()
            shot("movement-interrupted")
            phase("damage_start")
        elif p == "damage_start" and wait > 0.4:
            place(150)
            begin()
            unreal.GameplayStatics.apply_damage(st["pawn"], 0, st["pc"], None, unreal.DamageType)
            check("zero_damage_does_not_interrupt", st["interaction"].get_status() == S.RUNNING)
            phase("damage")
        elif p == "damage" and wait > 0.5:
            unreal.GameplayStatics.apply_damage(st["pawn"], 1, st["pc"], None, unreal.DamageType)
            check("damage_interrupts_without_charge", st["interaction"].get_status() == S.INTERRUPTED and st["inv"].get_item_count("wood") == 10)
            begin()
            phase("near_finish")
        elif p == "near_finish" and st["action"].get_elapsed_seconds() >= 4:
            check("no_early_completion", st["action"].get_elapsed_seconds() < 5 and st["entry"]["ready_callbacks"] == 0 and st["inv"].get_item_count("wood") == 10)
            place(250)
            phase("range_lost")
        elif p == "range_lost" and wait > 0.4:
            check("late_range_loss_cancels", st["interaction"].get_status() == S.OUT_OF_RANGE and st["entry"]["ready_callbacks"] == 0 and st["inv"].get_item_count("wood") == 10)
            place(150)
            begin()
            st["target_actor"].destroy_actor()
            phase("destroyed")
        elif p == "destroyed" and wait > 0.4:
            check("destroyed_target_cancels", st["interaction"].get_status() == S.INVALID_TARGET and st["entry"]["ready_callbacks"] == 0 and st["inv"].get_item_count("wood") == 10)
            create_target()
            st["hud"].toggle_inventory()
            check("paused_start_rejected", not st["interaction"].begin_interaction(st["target"]) and st["interaction"].get_status() == S.PAUSED)
            st["hud"].toggle_inventory()
            st["sub"].inject_input_vector_for_action(st["interact"], unreal.Vector(1, 0, 0), [], [])
            phase("input_started")
        elif p == "input_started" and wait > 0.4:
            check("e_action_starts_nearest", st["interaction"].get_status() == S.RUNNING)
            shot("running")
            phase("pause")
        elif p == "pause" and wait > 0.4:
            st["hud"].toggle_inventory()
            st.update(frozen=st["action"].get_elapsed_seconds(), frozen_clock=st["clock"].get_snapshot().active_play_seconds)
            phase("paused")
        elif p == "paused" and wait > 0.7:
            check("pause_freezes_action_and_clock", st["action"].get_elapsed_seconds() == st["frozen"] and st["clock"].get_snapshot().active_play_seconds == st["frozen_clock"] and st["interaction"].get_status() == S.RUNNING)
            check("paused_no_charge", st["inv"].get_item_count("wood") == 10 and st["entry"]["ready_callbacks"] == 0)
            st["hud"].toggle_inventory()
            phase("complete")
        elif p == "complete" and st["interaction"].get_status() == S.READY:
            check("complete_once_at_five_seconds", st["action"].get_elapsed_seconds() == 5 and st["entry"]["ready_callbacks"] == 1 and st["entry"]["settled"] == 1 and st["inv"].get_item_count("wood") == 8)
            shot("ready")
            phase("after_complete")
        elif p == "after_complete" and wait > 0.5:
            check("no_delayed_duplicate", st["entry"]["ready_callbacks"] == 1 and st["inv"].get_item_count("wood") == 8)
            assert st["inv"].try_remove("wood", 6) == R.SUCCESS
            begin()
            phase("consume_materials")
        elif p == "consume_materials" and wait > 0.5:
            assert st["inv"].try_remove("wood", 2) == R.SUCCESS
            check("no_resource_reservation", st["inv"].get_item_count("wood") == 0 and st["interaction"].get_status() == S.RUNNING)
            phase("missing_at_completion")
        elif p == "missing_at_completion" and st["interaction"].get_status() == S.READY:
            check("consumer_rechecks_materials", st["entry"]["ready_callbacks"] == 2 and st["entry"]["settled"] == 1 and st["inv"].get_item_count("wood") == 0 and "INSUFFICIENT_ITEMS" in st["entry"]["settlement_results"][-1])
            check("all_completions_at_five", st["entry"]["completion_elapsed"] == [5, 5])
            levels.editor_request_end_play()
            phase("stopped")
        elif p == "stopped" and not levels.is_in_play_in_editor() and wait > 1:
            if st["round"] == 2: finish()
            else: phase("start")
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
