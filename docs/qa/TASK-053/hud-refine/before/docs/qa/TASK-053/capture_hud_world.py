"""Native PIE new-game capture; isolated saves, actual world and actual HUD."""
import json
import os
from pathlib import Path
import time
import traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id = os.environ["HEARTHWARD_HUD_RUN"]
out = Path(unreal.Paths.project_saved_dir()) / "HUDPreview" / run_id
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report = {"run_id": run_id, "passed": False, "checks": {}, "captures": [], "layouts": []}


def check(name, result):
    report["checks"][name] = bool(result)
    if not result:
        raise AssertionError(name)


def delay(seconds):
    deadline = time.monotonic() + seconds
    return lambda: time.monotonic() >= deadline


def game_ready():
    world = editor.get_game_world()
    if not world or unreal.GameplayStatics.get_current_level_name(world, True) != "L_HearthwardWilds":
        return False
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    if not controller or not controller.get_hud() or not controller.get_hud().screen:
        return False
    loading = next((s for s in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem)
                    if s.get_outer() == unreal.GameplayStatics.get_game_instance(world)), None)
    return str(controller.get_hud().screen.get_page()) == "hud" and loading and not loading.is_loading()


def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world = editor.get_game_world()
    ui = unreal.GameplayStatics.get_player_controller(world, 0).get_hud().screen
    check("isolated_new_game_action", ui.execute_action("new"))
    yield game_ready
    yield delay(8)
    world = editor.get_game_world()
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    ui = controller.get_hud().screen
    check("natural_world_hud", str(ui.get_page()) == "hud")
    save = next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer() == world)
    check("natural_save_world_enabled", save.is_natural_world_enabled())
    report["map"] = unreal.GameplayStatics.get_current_level_name(world, True)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    report["player_position"] = str(pawn.get_actor_location())
    for index, name in enumerate(("medicine", "roast", "arrow", "firepot")):
        check("select_" + name, ui.execute_action("hud.quick.select:" + str(index)) and ui.get_hud_quick_selection() == index)
        report["layouts"].append(json.loads(ui.describe_layout()))
        yield delay(1)
        filename = "hud-" + name + ".png"
        unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.CaptureHUDScene hud-" + name, controller)
        yield lambda filename=filename: (out / filename).is_file()
        check("native_capture_" + name, (out / filename).stat().st_size > 50000)
        report["captures"].append(filename)
    ui.execute_action("hud.quick.select:0")
    levels.editor_request_end_play()
    yield lambda: not levels.is_in_play_in_editor()
    report["passed"] = True


iterator = run()
pending = None
deadline = time.monotonic() + 450


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError("Natural-world HUD preview capture")
        if pending and not pending():
            return
        pending = next(iterator)
    except StopIteration:
        (out / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report["error"] = traceback.format_exc()
        (out / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        unreal.unregister_slate_post_tick_callback(handle)


handle = unreal.register_slate_post_tick_callback(tick)
