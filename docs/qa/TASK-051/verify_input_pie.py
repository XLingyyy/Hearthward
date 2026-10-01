"""PIE fixture for observed physical key capture through the real settings UI.

The host agent presses keys with computer-use after ready.json is observed.
Sent markers synchronize observation only; persisted bindings and displacement
are asserted separately. Test state and configuration are isolated.
"""
import configparser
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
out = root / "Saved/Task051/input"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report = {"ok": False, "checks": {}, "method": __doc__}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def delay(seconds):
    until = time.monotonic() + seconds
    return lambda: time.monotonic() >= until


def request(stage, key):
    (out/"ready.json").write_text(json.dumps({"stage": stage, "key": key}), encoding="utf-8")
    return lambda: (out/(stage + ".sent")).exists()


def config():
    result = configparser.ConfigParser(strict=False)
    result.read(Path(unreal.Paths.project_saved_dir()) / "Config/WindowsEditor/GameUserSettings.ini", encoding="utf-8-sig")
    return result


def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(3)
    world = editor.get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    ui = pc.get_hud().get_editor_property("screen")
    unreal.GameplayStatics.set_game_paused(world, False)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    yield delay(.8)
    pawn.get_component_by_class(unreal.HearthwardGameplayComponent).set_editor_property("enabled", True)
    save = next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer() == world)
    check("isolated progress", save.enable_prototype() and save.start_new_progress())
    ui.open_page("settings")
    ui.execute_action("settings.defaults")
    check("default preferences applied", ui.execute_action("settings.apply"))
    ui.execute_action("cancel")
    ui.execute_action("category:键位")
    check("storage capture opens", ui.execute_action("settings.bind:storage.open:0"))
    yield request("storage", "y")
    yield delay(.5)
    check("captured storage key applies", ui.execute_action("settings.apply"))
    cfg = config()
    check("physical Y persisted", cfg.get("Hearthward.Bindings", "storage.open0") == "Y")
    check("other R semantics remain independent", cfg.get("Hearthward.Bindings", "combat.stun0") == "R" and cfg.get("Hearthward.Bindings", "combat.reload0") == "R")
    check("combination capture opens", ui.execute_action("settings.bind:ui.map:0"))
    yield request("chord", "Control_L+u")
    yield delay(.5)
    check("captured chord applies", ui.execute_action("settings.apply"))
    check("physical Ctrl-U persisted", config().get("Hearthward.Bindings", "ui.map0") == "LeftControl+U")
    check("movement capture opens", ui.execute_action("settings.bind:move.forward:0"))
    yield request("movebind", "i")
    yield delay(.5)
    check("captured movement applies", ui.execute_action("settings.apply"))
    check("physical I persisted", config().get("Hearthward.Bindings", "move.forward0") == "I")
    ui.open_page("hud")
    start = pawn.get_actor_location()
    yield request("move", "i")
    yield delay(.8)
    report["movement_cm"] = (pawn.get_actor_location() - start).length()
    check("remapped physical input moves character", report["movement_cm"] > 1)
    ui.open_page("settings")
    ui.execute_action("category:键位")
    (Path(unreal.Paths.project_saved_dir())/"Task020").mkdir(parents=True, exist_ok=True)
    check("captured binding UI", ui.capture_ui("TASK051-physical-bindings", 1920, 1080))
    report["bindings"] = {key: config().get("Hearthward.Bindings", key) for key in ["storage.open0", "combat.stun0", "combat.reload0", "ui.map0", "move.forward0"]}
    report["ok"] = True


runner = run()
pending = None
deadline = time.monotonic() + 300


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError("physical key fixture")
        if pending and not pending():
            return
        pending = next(runner)
        return
    except StopIteration:
        pass
    except Exception:
        report["error"] = traceback.format_exc()
    (out/"results.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()


handle = unreal.register_slate_post_tick_callback(tick)
