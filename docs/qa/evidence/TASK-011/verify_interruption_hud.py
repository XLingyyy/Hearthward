"""Focused visual check after removing duplicate interaction/timer interruption notices."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task011"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
st = {"phase": 0, "since": time.monotonic(), "deadline": time.monotonic() + 60}

def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    (out / "hud-results.json").write_text(json.dumps({"ok": not error, "error": error, "screenshot": "interruption-hud.png", "checks": st.get("checks", {})}, indent=2), encoding="utf-8")
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

def tick(dt):
    try:
        if time.monotonic() > st["deadline"]: raise TimeoutError("HUD check timed out")
        wait = time.monotonic() - st["since"]
        if st["phase"] == 0:
            levels.editor_request_begin_play()
            st.update(phase=1, since=time.monotonic())
        elif st["phase"] == 1 and wait > 1:
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
            if not pawn: return
            interaction = pawn.get_component_by_class(unreal.HearthwardInteractionComponent)
            unreal.SystemLibrary.execute_console_command(world, "Hearthward.Interaction.CreateTestTarget")
            actor = unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Interaction.PROTOTYPE_ONLY")[0]
            target = actor.get_component_by_class(unreal.HearthwardInteractionTargetComponent)
            pawn.get_movement_component().stop_movement_immediately()
            assert interaction.begin_interaction(target)
            action = pawn.get_component_by_class(unreal.HearthwardTimedActionComponent)
            st.update(phase=2, since=time.monotonic(), world=world, action=action, interaction=interaction)
        elif st["phase"] == 2 and wait > 0.5:
            st["action"].interrupt_action()
            st["checks"] = {"timer_interrupted": st["action"].get_status() == unreal.HearthwardTimedActionStatus.INTERRUPTED,
                            "interaction_interrupted": st["interaction"].get_status() == unreal.HearthwardInteractionStatus.INTERRUPTED}
            assert all(st["checks"].values())
            unreal.SystemLibrary.execute_console_command(st["world"], f'HighResShot 1280x720 filename="{(out / "interruption-hud.png").as_posix()}"')
            st.update(phase=3, since=time.monotonic())
        elif st["phase"] == 3 and wait > 0.5:
            finish()
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
