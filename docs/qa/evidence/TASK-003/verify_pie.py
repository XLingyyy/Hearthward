"""Native UE Python acceptance probe, launched through UEClient (two PIE sessions).

Injects the actual Enhanced Input actions; desktop key/mouse checks remain separate.
Results are written to Saved/Task003, never inferred from configured speed alone.
"""
import json
import math
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)

out = Path(unreal.Paths.project_dir()) / "Saved/Task003"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "input_method": "EnhancedInput action injection", "sessions": []}
state = {"phase": "start", "session": 0, "deadline": time.monotonic() + 180}
cases = [("forward", 0, 1), ("backward", 0, -1), ("right", 1, 0),
         ("left", -1, 0), ("diagonal", 1, 1), ("release", 0, 0),
         ("resume", 0, 1), ("camera", 0, 0), ("collision", 0, 1)]


def xyz(v):
    return [v.x, v.y, v.z]


def finish(error=None):
    if error:
        report["error"] = error
    else:
        report["ok"] = all(c["passed"] for s in report["sessions"] for c in s["cases"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "pie-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


def tick(dt):
    try:
        now = time.monotonic()
        if now > state["deadline"]:
            raise TimeoutError("PIE verification exceeded 180 seconds")
        phase = state["phase"]
        if phase == "start":
            state["session"] += 1
            levels.editor_request_begin_play()
            state.update(phase="possess", since=now)
        elif phase == "possess":
            if not levels.is_in_play_in_editor() or now - state["since"] < 2:
                return
            world = unreal.EditorLevelLibrary.get_game_world()
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0) if pc else None
            if not pawn:
                return
            actions = sorted((a for a in unreal.ObjectIterator(unreal.InputAction)
                              if a.get_outer() == pawn), key=lambda a: a.get_name())
            # SetupPlayerInputComponent creates Move first, then Look; no other actions.
            assert len(actions) == 2, "Unexpected input actions on bootstrap pawn"
            subsystems = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                          if isinstance(s.get_outer(), unreal.LocalPlayer)
                          and s.query_keys_mapped_to_action(actions[0])]
            assert len(subsystems) == 1, [s.get_path_name() for s in subsystems]
            sub = subsystems[0]
            state.update(pc=pc, pawn=pawn, sub=sub,
                         move=actions[0], look=actions[1], index=0, phase="prepare")
            report["sessions"].append({"number": state["session"], "pawn": pawn.get_class().get_name(), "cases": []})
        elif phase == "prepare":
            name, x, y = cases[state["index"]]
            pawn, pc = state["pawn"], state["pc"]
            if name not in ("release", "resume"):
                pawn.set_actor_location(unreal.Vector(0, 0, 95), False, True)
                pawn.character_movement.stop_movement_immediately()
                pc.set_control_rotation(unreal.Rotator(0, 0, 0))
            state.update(phase="sample", elapsed=0.0, samples=[], start=xyz(pawn.get_actor_location()),
                         start_rotation=[pc.get_control_rotation().yaw, pc.get_control_rotation().pitch])
        elif phase == "sample":
            name, x, y = cases[state["index"]]
            pawn, pc = state["pawn"], state["pc"]
            state["sub"].inject_input_vector_for_action(state["move"], unreal.Vector(x, y, 0), [], [])
            if name == "camera":
                state["sub"].inject_input_vector_for_action(state["look"], unreal.Vector(0.5, 0.15, 0), [], [])
            state["elapsed"] += dt
            speed = math.hypot(pawn.get_velocity().x, pawn.get_velocity().y)
            if state["elapsed"] > 0.6:
                state["samples"].append(speed)
            duration = 4.0 if name == "collision" else 1.25
            if state["elapsed"] < duration:
                return
            pos = xyz(pawn.get_actor_location())
            values = state["samples"]
            mean = sum(values) / len(values)
            if name == "collision":
                passed = 910 <= pos[0] <= 917 and speed < 1
            elif name == "release":
                passed = max(values) < 1
            elif name == "camera":
                passed = abs(pc.get_control_rotation().yaw - state["start_rotation"][0]) > 10
            else:
                displacement = [pos[i] - state["start"][i] for i in range(3)]
                direction = displacement[0] * y + displacement[1] * x
                passed = abs(mean - 350) <= 5 and max(values) <= 355 and direction > 100
            report["sessions"][-1]["cases"].append({
                "name": name, "passed": passed, "sample_count": len(values),
                "mean_speed_cm_s": mean, "max_speed_cm_s": max(values),
                "final_speed_cm_s": speed, "start_cm": state["start"], "end_cm": pos,
                "control_rotation": [pc.get_control_rotation().yaw, pc.get_control_rotation().pitch]})
            state["index"] += 1
            if state["index"] < len(cases):
                state["phase"] = "prepare"
            else:
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
