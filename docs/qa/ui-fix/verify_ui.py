import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "UIFix"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"passed": False, "checks": {}}


def check(name, result):
    report["checks"][name] = bool(result)
    if not result:
        raise AssertionError(name)


def delay(seconds):
    end = time.monotonic() + seconds
    return lambda: time.monotonic() >= end


def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    ui = controller.get_hud().screen
    save = next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == world)
    check("title_16_9", ui.capture_ui("ui-fix-title-16x9", 1672, 941))
    check("title_16_10", ui.capture_ui("ui-fix-title-16x10", 2560, 1600))
    check("title_ultrawide", ui.capture_ui("ui-fix-title-21x9", 2520, 1080))
    check("open_settings", ui.execute_action("page:settings"))
    check("settings_16_10", ui.capture_ui("ui-fix-settings-16x10", 2560, 1600))
    for category in ("游戏", "显示", "图形", "音频", "控制", "键位", "辅助功能", "教程"):
        check("tab_" + category, ui.execute_action("category:" + category))
        check("capture_" + category, ui.capture_ui("ui-fix-settings-" + category, 1672, 941))
    check("back_to_game_settings", ui.execute_action("category:游戏"))
    old_interval = save.get_auto_minutes()
    change = -1 if old_interval == 60 else 1
    check("change_draft", ui.execute_action("settings.change:autosave:" + str(change)))
    check("draft_not_committed", save.get_auto_minutes() == old_interval)
    check("apply_settings", ui.execute_action("settings.apply"))
    check("save_interval_committed", save.get_auto_minutes() == old_interval + change)
    check("restore_draft", ui.execute_action("settings.change:autosave:" + str(-change)))
    check("restore_setting", ui.execute_action("settings.apply"))
    check("save_interval_restored", save.get_auto_minutes() == old_interval)
    check("return_title", ui.execute_action("back"))
    check("returned_to_title", str(ui.get_page()) == "title")
    levels.editor_request_end_play()
    yield lambda: not levels.is_in_play_in_editor()
    check("pie_teardown", True)
    report["passed"] = True
    (out / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


iterator = run()
pending = None
deadline = time.monotonic() + 180


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError("UI validation")
        if pending and not pending():
            return
        pending = next(iterator)
    except StopIteration:
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report["error"] = traceback.format_exc()
        (out / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        unreal.unregister_slate_post_tick_callback(handle)


handle = unreal.register_slate_post_tick_callback(tick)
