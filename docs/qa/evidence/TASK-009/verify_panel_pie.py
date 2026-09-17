"""Two PIE sessions; input-action injection, native pause and rendered HUD checks."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task009"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "sessions": [], "input_method": "Enhanced Input action injection; mapped Tab inspected, no physical keyboard claim"}
st = {"phase": "start", "round": 0, "deadline": time.monotonic() + 150}

def phase(name):
    st.update(phase=name, since=time.monotonic(), frames=0)

def inject(action, x=0, y=0):
    st["sub"].inject_input_vector_for_action(st[action], unreal.Vector(x, y, 0), [], [])

def check(name, value):
    st["entry"]["checks"][name] = bool(value)

def clock():
    return st["clock"].get_snapshot().active_play_seconds

def shot(label, size="1280x720"):
    if st["round"] != 1: return
    path = (out / (label + ".png")).as_posix()
    unreal.SystemLibrary.execute_console_command(st["world"], f'HighResShot {size} filename="{path}"', st["pc"])

def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and len(report["sessions"]) == 2 and all(all(e["checks"].values()) for e in report["sessions"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

def tick(dt):
    try:
        if time.monotonic() > st["deadline"]: raise TimeoutError("Panel PIE timed out")
        p = st["phase"]
        st["frames"] = st.get("frames", 0) + 1
        # HighResShot can occupy the entire wall-time delay; allow input-release frames too.
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
            subs = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(s.get_outer(), unreal.LocalPlayer)]
            bindings = [(s, a) for s in subs for a in actions if len(s.query_keys_mapped_to_action(a)) == 4]
            if not bindings: return
            sub, move = bindings[0]
            toggle = next(a for a in actions if a.get_name() == "InventoryToggleAction")
            look = next(a for a in actions if a != move and a != toggle)
            e = {"round": st["round"], "checks": {}}
            report["sessions"].append(e)
            st.update(world=world, pawn=pawn, pc=pc, hud=pc.get_hud(), entry=e, sub=sub, move=move, toggle=toggle, look=look,
                      inv=pawn.get_component_by_class(unreal.HearthwardInventoryComponent),
                      action=pawn.get_component_by_class(unreal.HearthwardTimedActionComponent),
                      clock=next(c for c in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if c.get_outer() == world))
            check("fresh_closed_empty_running", not st["hud"].is_inventory_open() and st["inv"].get_weight() == 0 and not unreal.GameplayStatics.is_game_paused(world))
            keys = sub.query_keys_mapped_to_action(toggle)
            e["mapped_keys"] = [str(k.export_text()) for k in keys]
            check("tab_mapping", len(keys) == 1 and "Tab" in e["mapped_keys"][0])
            phase("settle")
        elif p == "settle" and wait > 0.5:
            inject("toggle", 1)
            phase("empty_open")
        elif p == "empty_open":
            inject("toggle", 1)
            if wait < 0.4: return
            check("held_once_open_paused", st["hud"].is_inventory_open() and unreal.GameplayStatics.is_game_paused(st["world"]))
            shot("empty")
            inject("toggle")
            phase("empty_capture")
        elif p == "empty_capture" and wait > 0.3:
            inject("toggle", 1)
            phase("empty_close")
        elif p == "empty_close" and wait > 0.3:
            check("paused_input_closes_and_resumes", not st["hud"].is_inventory_open() and not unreal.GameplayStatics.is_game_paused(st["world"]))
            inv = st["inv"]
            for item, count in [("wood", 10), ("stone", 5), ("ore", 3), ("meat", 4), ("arrow", 7)]:
                assert inv.try_add(item, count) == unreal.HearthwardInventoryResult.SUCCESS
            check("mixed_weight", inv.get_weight() == 23.35)
            assert st["action"].start_action()
            phase("action_running")
        elif p == "action_running" and wait > 0.6:
            inject("toggle", 1)
            phase("mixed_open")
        elif p == "mixed_open" and wait > 0.2:
            check("action_panel_open", st["hud"].is_inventory_open())
            st.update(frozen_clock=clock(), frozen_action=st["action"].get_elapsed_seconds(), position=st["pawn"].get_actor_location(), rotation=st["pc"].get_control_rotation())
            shot("mixed-paused")
            phase("frozen")
        elif p == "frozen":
            inject("move", 0, 1)
            inject("look", 1, 1)
            if wait < 0.7: return
            check("clock_frozen", clock() == st["frozen_clock"])
            check("action_frozen", st["action"].get_elapsed_seconds() == st["frozen_action"] and st["action"].get_status() == unreal.HearthwardTimedActionStatus.RUNNING)
            check("movement_frozen", st["pawn"].get_actor_location() == st["position"])
            check("look_frozen", st["pc"].get_control_rotation() == st["rotation"])
            inject("move")
            inject("look")
            assert st["inv"].try_add("arrow", 1533) == unreal.HearthwardInventoryResult.SUCCESS
            check("full_weight", st["inv"].get_weight() == 100)
            phase("full")
        elif p == "full" and wait > 0.3:
            shot("full-paused", "1920x1080")
            phase("full_capture")
        elif p == "full_capture" and wait > 0.3:
            inject("toggle", 1)
            phase("resumed")
        elif p == "resumed" and wait > 0.4:
            check("clock_resumed", clock() > st["frozen_clock"])
            check("action_resumed", st["action"].get_elapsed_seconds() > st["frozen_action"] and st["action"].get_status() == unreal.HearthwardTimedActionStatus.RUNNING)
            check("closed", not st["hud"].is_inventory_open())
            shot("closed-running")
            phase("move_again")
        elif p == "move_again":
            inject("move", 0, 1)
            if wait < 0.7: return
            speed = st["pawn"].get_velocity().length()
            st["entry"]["resumed_full_load_speed"] = speed
            check("movement_resumed_full_load", abs(speed - 315) < 0.5)
            check("move_interrupts_after_resume", st["action"].get_status() == unreal.HearthwardTimedActionStatus.INTERRUPTED)
            inject("move")
            st["pawn"].get_movement_component().stop_movement_immediately()
            assert unreal.GameplayStatics.set_game_paused(st["world"], True)
            inject("toggle", 1)
            phase("external_open")
        elif p == "external_open" and wait > 0.3:
            check("opens_while_already_paused", st["hud"].is_inventory_open())
            inject("toggle", 1)
            phase("external_close")
        elif p == "external_close" and wait > 0.3:
            check("preserves_external_pause", not st["hud"].is_inventory_open() and unreal.GameplayStatics.is_game_paused(st["world"]))
            assert unreal.GameplayStatics.set_game_paused(st["world"], False)
            levels.editor_request_end_play()
            phase("stopped")
        elif p == "stopped" and not levels.is_in_play_in_editor() and wait > 1:
            if st["round"] == 2: finish()
            else: phase("start")
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
