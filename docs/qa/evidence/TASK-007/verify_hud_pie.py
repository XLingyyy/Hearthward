"""Native HUD rendering checks; launch using UEClient."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task007"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "sessions": []}
st = {"phase": "start", "round": 0, "deadline": time.monotonic() + 120}

def phase(name):
    st.update(phase=name, since=time.monotonic())

def shot(label):
    size = "1280x720" if st["round"] == 1 else "1920x1080"
    unreal.SystemLibrary.execute_console_command(st["world"],
        f'HighResShot {size} filename="{(out / (str(st["round"]) + "-" + label)).as_posix()}.png"', st["pc"])
    st["entry"]["captures"].append({"label": label, "render_size": size,
        "state": str(st["action"].get_status()), "elapsed": st["action"].get_elapsed_seconds(),
        "paused": unreal.GameplayStatics.is_game_paused(st["world"])})

def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and len(report["sessions"]) == 2 and all(all(e["checks"].values()) for e in report["sessions"])
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "hud-results.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor():
        unreal.GameplayStatics.set_game_paused(st["world"], False)
        levels.editor_request_end_play()

def tick(dt):
    try:
        if time.monotonic() > st["deadline"]: raise TimeoutError("HUD PIE timed out")
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
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            a = pawn.get_component_by_class(unreal.HearthwardTimedActionComponent)
            e = {"round": st["round"], "captures": [], "checks": {
                "hud_class": isinstance(pc.get_hud(), unreal.HearthwardHUD),
                "fresh_idle": a.get_status() == unreal.HearthwardTimedActionStatus.IDLE}}
            report["sessions"].append(e)
            st.update(world=world, pawn=pawn, pc=pc, action=a, entry=e)
            phase("idle")
        elif p == "idle" and wait > 0.5:
            shot("idle")
            phase("begin")
        elif p == "begin" and wait > 0.25:
            assert st["action"].start_action()
            phase("running")
        elif p == "running" and st["action"].get_elapsed_seconds() > 1.0:
            shot("running")
            phase("pause_begin")
        elif p == "pause_begin" and wait > 0.25:
            assert unreal.GameplayStatics.set_game_paused(st["world"], True)
            st["paused_elapsed"] = st["action"].get_elapsed_seconds()
            phase("paused")
        elif p == "paused" and wait > 2:
            st["entry"]["checks"]["pause_freezes"] = st["action"].get_elapsed_seconds() == st["paused_elapsed"]
            shot("paused")
            phase("resume")
        elif p == "resume" and wait > 0.25:
            assert unreal.GameplayStatics.set_game_paused(st["world"], False)
            phase("completing")
        elif p == "completing" and st["action"].get_status() == unreal.HearthwardTimedActionStatus.COMPLETED:
            st["entry"]["checks"]["real_completion"] = st["action"].get_elapsed_seconds() == 5
            phase("completed_capture")
        elif p == "completed_capture" and wait > 0.25:
            shot("completed")
            phase("restart")
        elif p == "restart" and wait > 0.25:
            assert st["action"].start_action()
            phase("damage")
        elif p == "damage" and wait > 0.7:
            unreal.GameplayStatics.apply_damage(st["pawn"], 1, st["pc"], None, unreal.DamageType)
            st["entry"]["checks"]["damage_interrupts"] = st["action"].get_status() == unreal.HearthwardTimedActionStatus.INTERRUPTED
            phase("interrupted_capture")
        elif p == "interrupted_capture" and wait > 0.2:
            shot("interrupted")
            phase("notice_expire")
        elif p == "notice_expire" and wait > 2:
            shot("expired")
            phase("end")
        elif p == "end" and wait > 0.25:
            levels.editor_request_end_play()
            phase("stopped")
        elif p == "stopped" and not levels.is_in_play_in_editor() and wait > 1:
            if st["round"] == 2: finish()
            else: phase("start")
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
