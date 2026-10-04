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
report = {"run_id": run_id, "passed": False, "checks": {}, "captures": [], "layouts": [], "attachments": []}


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
    world = editor.get_game_world()
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    ui = controller.get_hud().screen
    yield lambda: ui.get_hud_quest_notice_remaining() > 0
    yield delay(.3)
    ui.refresh()
    layout = json.loads(ui.describe_layout())
    elements = {row["id"]: row for row in layout["components"] if "text" in row}
    report["quest_remaining_at_receipt"] = ui.get_hud_quest_notice_remaining()
    check("new_game_receives_visible_five_second_quest", 4 < report["quest_remaining_at_receipt"] <= 5 and "hud.quest.heading" in elements)
    check("companion_above_received_quest", elements["hud.companion.order"]["rect"][1] + elements["hud.companion.order"]["rect"][3] < elements["hud.quest.heading"]["rect"][1])
    report["layouts"].append(layout)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.CaptureHUDScene hud-task-received", controller)
    yield lambda: (out / "hud-task-received.png").is_file()
    check("native_capture_received_quest", (out / "hud-task-received.png").stat().st_size > 50000)
    report["captures"].append("hud-task-received.png")
    yield delay(5.2)
    ui.refresh()
    elements = {row["id"]: row for row in json.loads(ui.describe_layout())["components"] if "text" in row}
    report["quest_remaining_after_wait"] = ui.get_hud_quest_notice_remaining()
    check("quest_disappears_after_five_real_seconds", report["quest_remaining_after_wait"] == 0 and "hud.quest.heading" not in elements and "hud.quest.objective" not in elements)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.CaptureHUDScene hud-task-hidden", controller)
    yield lambda: (out / "hud-task-hidden.png").is_file()
    check("native_capture_hidden_quest", (out / "hud-task-hidden.png").stat().st_size > 50000)
    report["captures"].append("hud-task-hidden.png")
    check("journal_entry", ui.execute_action("page:journal") and str(ui.get_page()) == "journal")
    journal = json.loads(ui.describe_layout())
    check("quest_details_remain_in_journal", any("从家中的遗物包" in row.get("text", "") for row in journal["components"]))
    check("return_to_hud", ui.execute_action("page:hud") and str(ui.get_page()) == "hud")
    check("return_from_journal_does_not_replay_old_quest", ui.get_hud_quest_notice_remaining() == 0)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.VerifyHUDHintRange", controller)
    yield lambda: (out / "hint-range.json").is_file()
    ranges = json.loads((out / "hint-range.json").read_text(encoding="utf-8"))
    check("native_ten_metre_hint_checks", ranges["passed"] and all(ranges["checks"].values()))
    report["attachments"].append("hint-range.json")
    yield delay(1)
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
    check("inventory_entry", ui.execute_action("page:inventory") and str(ui.get_page()) == "inventory")
    for index, category in enumerate(("装备", "材料", "食物", "工具")):
        check("inventory_tab_" + str(index), ui.execute_action("filter:" + category) and ui.get_category() == category)
        report["layouts"].append(json.loads(ui.describe_layout()))
        yield delay(.4)
        name = "inventory-" + str(index)
        unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.CaptureHUDScene " + name, controller)
        yield lambda name=name: (out / (name + ".png")).is_file()
        check("native_capture_" + name, (out / (name + ".png")).stat().st_size > 50000)
        report["captures"].append(name + ".png")
    check("inventory_back_returns_hud", ui.execute_action("back") and str(ui.get_page()) == "hud")
    check("inventory_does_not_replay_old_quest", ui.get_hud_quest_notice_remaining() == 0)
    check("skills_entry", ui.execute_action("page:skills") and str(ui.get_page()) == "skills")
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    original_points = gameplay.skill_points()
    check("natural_skill_fixture_starts_unlearned", not dict(gameplay.get_editor_property("skills").items()))
    check("skills_select_original_root", ui.execute_action("skill:strong"))
    for name, action in (("skills-default", None), ("skills-sense", "skill:trail")):
        if action:
            check(name + "_selection", ui.execute_action(action))
        report["layouts"].append(json.loads(ui.describe_layout()))
        yield delay(.4)
        unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.CaptureHUDScene " + name, controller)
        yield lambda name=name: (out / (name + ".png")).is_file()
        check("native_capture_" + name, (out / (name + ".png")).stat().st_size > 50000)
        report["captures"].append(name + ".png")
    ui.execute_action("skill:strong")
    check("natural_root_learning_uses_original_points", ui.execute_action("learn") and gameplay.skill_points() == original_points - 1)
    ui.execute_action("skill:vigor")
    check("natural_child_learning_after_prerequisite", ui.execute_action("learn") and gameplay.skill_points() == original_points - 2)
    report["layouts"].append(json.loads(ui.describe_layout()))
    yield delay(.4)
    name = "skills-learned"
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.CaptureHUDScene " + name, controller)
    yield lambda: (out / (name + ".png")).is_file()
    check("native_capture_" + name, (out / (name + ".png")).stat().st_size > 50000)
    report["captures"].append(name + ".png")
    # Use the original free-respec action; original event receipts remain in this disposable test pool.
    check("natural_free_respec_refunds_points", ui.execute_action("respec") and gameplay.skill_points() == original_points
          and not dict(gameplay.get_editor_property("skills").items()))
    check("skills_back_returns_hud", ui.execute_action("back") and str(ui.get_page()) == "hud")
    check("skills_does_not_replay_old_quest", ui.get_hud_quest_notice_remaining() == 0)
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
