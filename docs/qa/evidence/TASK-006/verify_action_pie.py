"""Native PIE verification, launched through UEClient. No assets are changed."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task006"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "sessions": []}
st = {"phase": "start", "round": 0, "deadline": time.monotonic() + 150}

def complete():
    st["entry"]["completed_events"] += 1

def interrupted():
    st["entry"]["interrupted_events"] += 1

def status():
    return st["action"].get_status()

def active():
    return st["clock"].get_snapshot().active_play_seconds

def capture(label):
    unreal.SystemLibrary.execute_console_command(st["world"], "Hearthward.Action", st["pc"])
    unreal.SystemLibrary.execute_console_command(st["world"],
        f'Shot SHOWUI filename="{(out / label).as_posix()}.png" -nosuffix', st["pc"])

def inject(action, x=0, y=0):
    st["sub"].inject_input_vector_for_action(action, unreal.Vector(x, y, 0), [], [])

def phase(name):
    st.update(phase=name, since=time.monotonic())

def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and len(report["sessions"]) == 2 and all(
        all(e["checks"].values()) for e in report["sessions"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor():
        unreal.GameplayStatics.set_game_paused(st["world"], False)
        levels.editor_request_end_play()

def tick(dt):
    try:
        now = time.monotonic()
        if now > st["deadline"]: raise TimeoutError("Timed action PIE verification timeout")
        p = st["phase"]
        if p == "start":
            st["round"] += 1
            levels.editor_request_begin_play()
            phase("possess")
        elif p == "possess":
            if not levels.is_in_play_in_editor(): return
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not pawn: return
            action = pawn.get_component_by_class(unreal.HearthwardTimedActionComponent)
            clock = next(c for c in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if c.get_outer() == world)
            actions = sorted((a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer() == pawn), key=lambda a: a.get_name())
            sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                       if isinstance(s.get_outer(), unreal.LocalPlayer) and s.query_keys_mapped_to_action(actions[0]))
            entry = {"round": st["round"], "completed_events": 0, "interrupted_events": 0, "checks": {}}
            report["sessions"].append(entry)
            st.update(world=world, pawn=pawn, pc=unreal.GameplayStatics.get_player_controller(world, 0),
                      action=action, clock=clock, move=actions[0], look=actions[1], sub=sub, entry=entry)
            action.on_timer_completed.add_callable(complete)
            action.on_interrupted.add_callable(interrupted)
            entry["checks"]["fresh_idle"] = status() == unreal.HearthwardTimedActionStatus.IDLE and action.get_elapsed_seconds() == 0
            st["started"] = active()
            unreal.SystemLibrary.execute_console_command(world, "Hearthward.Action.Start", st["pc"])
            entry["checks"]["console_starts"] = status() == unreal.HearthwardTimedActionStatus.RUNNING
            phase("initial_run")
        elif p == "initial_run":
            inject(st["look"], 0.2, 0)
            if active() - st["started"] < 1.0: return
            e = st["entry"]
            e["checks"]["look_does_not_interrupt"] = status() == unreal.HearthwardTimedActionStatus.RUNNING
            e["checks"]["duplicate_rejected"] = not st["action"].start_action()
            e["before_pause"] = st["action"].get_elapsed_seconds()
            e["clock_before_pause"] = active()
            assert unreal.GameplayStatics.set_game_paused(st["world"], True)
            e["checks"]["paused_start_rejected"] = not st["action"].start_action()
            phase("paused")
        elif p == "paused":
            if now - st["since"] < 2.5: return
            e = st["entry"]
            e["after_pause"] = st["action"].get_elapsed_seconds()
            e["paused_wall_seconds"] = now - st["since"]
            e["checks"]["pause_freezes"] = e["before_pause"] == e["after_pause"] and active() == e["clock_before_pause"]
            capture(f'round{st["round"]}-paused')
            assert unreal.GameplayStatics.set_game_paused(st["world"], False)
            phase("completion")
        elif p == "completion":
            if status() != unreal.HearthwardTimedActionStatus.COMPLETED: return
            e = st["entry"]
            e["completion_active_duration"] = active() - st["started"]
            e["checks"]["five_active_seconds"] = 5 <= e["completion_active_duration"] < 5.25
            e["checks"]["completed_clamped"] = st["action"].get_elapsed_seconds() == 5
            capture(f'round{st["round"]}-completed')
            phase("terminal_wait")
        elif p == "terminal_wait":
            if now - st["since"] < 0.6: return
            st["entry"]["checks"]["completion_once"] = st["entry"]["completed_events"] == 1
            assert st["action"].start_action()
            st["entry"]["checks"]["restart_clears_progress"] = st["action"].get_elapsed_seconds() == 0
            st["position"] = st["pawn"].get_actor_location()
            phase("move")
        elif p == "move":
            inject(st["move"], 0, 1)
            if now - st["since"] < 0.8: return
            inject(st["move"])
            e = st["entry"]
            e["checks"]["movement_interrupts"] = status() == unreal.HearthwardTimedActionStatus.INTERRUPTED and e["interrupted_events"] == 1
            e["checks"]["movement_regression"] = (st["pawn"].get_actor_location() - st["position"]).length() > 100
            capture(f'round{st["round"]}-interrupted')
            phase("settle")
        elif p == "settle":
            if now - st["since"] < 0.5: return
            assert st["action"].start_action()
            unreal.GameplayStatics.apply_damage(st["pawn"], 0, st["pc"], None, unreal.DamageType)
            st["entry"]["checks"]["zero_damage_ignored"] = status() == unreal.HearthwardTimedActionStatus.RUNNING
            phase("damage")
        elif p == "damage":
            if now - st["since"] < 0.5: return
            unreal.GameplayStatics.apply_damage(st["pawn"], 1, st["pc"], None, unreal.DamageType)
            e = st["entry"]
            e["checks"]["positive_damage_interrupts"] = status() == unreal.HearthwardTimedActionStatus.INTERRUPTED and e["interrupted_events"] == 2
            st["action"].interrupt_action()
            e["checks"]["interruption_once"] = e["interrupted_events"] == 2
            assert st["action"].start_action()
            st["restarted"] = active()
            phase("retry")
        elif p == "retry":
            e = st["entry"]
            if active() - st["restarted"] < 4.5:
                assert status() == unreal.HearthwardTimedActionStatus.RUNNING
                return
            if status() != unreal.HearthwardTimedActionStatus.COMPLETED: return
            e["retry_active_duration"] = active() - st["restarted"]
            e["checks"]["retry_full_duration"] = 5 <= e["retry_active_duration"] < 5.25 and e["completed_events"] == 2
            levels.editor_request_end_play()
            phase("stopped")
        elif p == "stopped" and not levels.is_in_play_in_editor() and now - st["since"] > 1:
            if st["round"] == 2: finish()
            else: phase("start")
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
