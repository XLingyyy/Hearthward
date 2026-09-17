"""Run in UE through UEClient.runtime.launch_editor -ExecutePythonScript.

Exercises actual world ticking and pause/resume, twice in one Editor process.
"""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task005"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "sessions": []}
state = {"phase": "start", "session": 0, "deadline": time.monotonic() + 120}


def read_clock():
    s = state["clock"].get_snapshot()
    return {"active_seconds": s.active_play_seconds,
            "calendar_minutes": s.elapsed_calendar_minutes,
            "days": s.elapsed_days, "minute_of_day": s.minute_of_day,
            "world_seconds": unreal.GameplayStatics.get_time_seconds(state["world"])}


def capture(name):
    unreal.SystemLibrary.execute_console_command(state["world"], "Hearthward.Clock", state["pc"])
    unreal.SystemLibrary.execute_console_command(
        state["world"], f'Shot SHOWUI filename="{(out / name).as_posix()}.png" -nosuffix', state["pc"])


def finish(error=None):
    if error:
        report["error"] = error
    report["ok"] = not error and len(report["sessions"]) == 2 and all(
        all(s["checks"].values()) for s in report["sessions"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor():
        unreal.GameplayStatics.set_game_paused(state["world"], False)
        levels.editor_request_end_play()


def tick(dt):
    try:
        now = time.monotonic()
        if now > state["deadline"]:
            raise TimeoutError("Clock PIE verification exceeded 120 seconds")
        phase = state["phase"]
        if phase == "start":
            editor_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
            assert not any(c.get_outer() == editor_world for c in unreal.ObjectIterator(
                unreal.HearthwardWorldClockSubsystem)), "Clock unexpectedly exists in editor world"
            state["session"] += 1
            levels.editor_request_begin_play()
            state["phase"] = "possess"
        elif phase == "possess":
            if not levels.is_in_play_in_editor():
                return
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not pc or not pawn:
                return
            clock = next(c for c in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem)
                         if c.get_outer() == world)
            actions = sorted((a for a in unreal.ObjectIterator(unreal.InputAction)
                              if a.get_outer() == pawn), key=lambda a: a.get_name())
            sub = next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                       if isinstance(s.get_outer(), unreal.LocalPlayer)
                       and s.query_keys_mapped_to_action(actions[0]))
            state.update(world=world, pc=pc, pawn=pawn, clock=clock, sub=sub, move=actions[0],
                         phase="running", since=now, start_x=pawn.get_actor_location().x)
            start = read_clock()
            state["entry"] = {"number": state["session"], "initial": start, "checks": {
                "fresh_world": 0 <= start["active_seconds"] < 0.5,
                "editor_world_excluded": True}}
            report["sessions"].append(state["entry"])
        elif phase == "running":
            state["sub"].inject_input_vector_for_action(state["move"],
                unreal.Vector(0, 1 if now-state["since"] < 1.5 else 0, 0), [], [])
            if now - state["since"] < 3:
                return
            current = read_clock()
            start = state["entry"]["initial"]
            checks = state["entry"]["checks"]
            checks["running_advances"] = current["active_seconds"] - start["active_seconds"] > 1
            checks["world_delta_matches"] = abs((current["active_seconds"]-start["active_seconds"])
                - (current["world_seconds"]-start["world_seconds"])) < 0.05
            checks["calendar_ratio"] = abs(current["active_seconds"]-current["calendar_minutes"]) < 0.000001
            checks["movement_regression"] = state["pawn"].get_actor_location().x - state["start_x"] > 100
            state["entry"]["before_pause"] = current
            capture(f'session{state["session"]}-running')
            assert unreal.GameplayStatics.set_game_paused(state["world"], True)
            state.update(phase="paused", since=now)
        elif phase == "paused":
            if now - state["since"] < 2.5:
                return
            current = read_clock()
            state["entry"]["after_pause"] = current
            state["entry"]["paused_wall_seconds"] = now - state["since"]
            before = state["entry"]["before_pause"]
            state["entry"]["checks"]["pause_freezes"] = current == before
            capture(f'session{state["session"]}-paused')
            assert unreal.GameplayStatics.set_game_paused(state["world"], False)
            state.update(phase="resumed", since=now)
        elif phase == "resumed":
            if now - state["since"] < 2.5:
                return
            current = read_clock()
            previous = state["entry"]["after_pause"]
            state["entry"]["after_resume"] = current
            delta = current["active_seconds"]-previous["active_seconds"]
            state["entry"]["checks"]["resume_no_catchup"] = delta > 1 and abs(
                delta - (current["world_seconds"]-previous["world_seconds"])) < 0.05
            capture(f'session{state["session"]}-resumed')
            state.update(phase="capture_wait", since=now)
        elif phase == "capture_wait" and now-state["since"] > 0.5:
            levels.editor_request_end_play()
            state.update(phase="stopped", since=now)
        elif phase == "stopped" and not levels.is_in_play_in_editor() and now-state["since"] > 1:
            if state["session"] == 2:
                finish()
            else:
                state["phase"] = "start"
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
