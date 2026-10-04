"""Real PIE checks for title navigation, modal ownership and aspect-ratio rendering."""
import json
import os
from pathlib import Path
import time
import traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id = os.environ["HEARTHWARD_TITLE_RUN"]
out = Path(unreal.Paths.project_saved_dir()) / "TitleWheel" / run_id
out.mkdir(parents=True, exist_ok=True)
(Path(unreal.Paths.project_saved_dir()) / "Task020").mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"passed": False, "run_id": run_id, "checks": {}, "captures": []}


def check(name, result):
    report["checks"][name] = bool(result)
    if not result:
        raise AssertionError(name)


def delay(seconds):
    end = time.monotonic() + seconds
    return lambda: time.monotonic() >= end


def capture(ui, name, width=1672, height=941):
    filename = run_id + "-" + name
    check("capture_" + name, ui.capture_ui(filename, width, height))
    report["captures"].append(filename + ".png")


def menu(ui):
    rows = json.loads(ui.describe_layout())["components"]
    options = [row for row in rows if row["id"].startswith("title.option.") and row["visible"]]
    arrows = {row["action"] for row in rows if row["id"].startswith("title.arrow.") and row["visible"]}
    return [row["action"] for row in options], arrows


def rows(ui, prefix):
    return [r for r in json.loads(ui.describe_layout())["components"]
            if r.get("action", "").startswith(prefix) and r["visible"]]


def verify_subpages(ui, world):
    check("styled_settings_entry", ui.execute_action("page:settings"))
    layout = json.loads(ui.describe_layout())["components"]
    check("wide_settings_sheet", next(r for r in layout if r["id"] == "settings.sheet")["rect"][2] == 1544)
    check("settings_background_art_removed", not any(r.get("asset") in {"pauseBackground", "leatherPanel"} for r in layout))
    check("eight_horizontal_categories", len(rows(ui, "category:")) == 8 and len({r["rect"][1] for r in rows(ui, "category:")}) == 1)
    for category in ("游戏", "显示", "图形", "音频", "控制", "键位", "辅助功能", "教程"):
        check("settings_category_" + category, ui.execute_action("category:" + category))
        check("settings_rows_inside_sheet_" + category, all(286 <= r["rect"][1] and r["rect"][1]+r["rect"][3] <= 688 for r in rows(ui, "settings.select:")))
        capture(ui, "settings-" + category)
    ui.execute_action("category:键位")
    check("seven_binding_rows", len(rows(ui, "settings.bind:")) == 14)
    check("settings_next_page_hit", ui.action_at(unreal.Vector2D(1460, 710)) == "settings.next")
    check("binding_paging", ui.execute_action("settings.next") and len(rows(ui, "settings.bind:")) == 14)
    capture(ui, "settings-binding-next")
    ui.execute_action("settings.prev")
    binding = rows(ui, "settings.bind:")[0]
    check("binding_capture_entry", ui.execute_action(binding["action"]))
    capture(ui, "settings-binding-capture")
    # Changing category cancels capture when leaving the page; no binding is edited here.
    ui.execute_action("back")
    check("binding_return_title", str(ui.get_page()) == "title")
    check("styled_empty_save_entry", ui.execute_action("page:save"))
    layout = json.loads(ui.describe_layout())["components"]
    check("wide_save_sheet", next(r for r in layout if r["id"] == "save.sheet")["rect"][2] == 1544)
    check("save_background_art_removed", not any(r.get("asset") in {"pauseBackground", "leatherPanel"} for r in layout))
    check("empty_save_message", any(r["id"] == "save.empty" for r in layout))
    capture(ui, "save-empty", 2560, 1600)
    check("save_from_title_disabled", ui.action_at(unreal.Vector2D(1420, 180)) == "")
    ui.execute_action("back")
    check("empty_save_returns_title", str(ui.get_page()) == "title")
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", controller)
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    player.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
    save = next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer() == world)
    check("isolated_save_fixture_enabled", save.enable_prototype())
    check("isolated_progress_created", save.start_new_progress())
    for i in range(9):
        check("isolated_save_" + str(i), save.save_point(i % 3 != 0))
    point_id = save.get_points()[-1].save_id
    parent_page = str(ui.get_page())
    report["fixture_parent_page"] = parent_page
    check("styled_populated_save_entry", ui.execute_action("page:save"))
    check("seven_visible_save_rows", len(rows(ui, "ask:load:")) == 7)
    report["styled_save_layout"] = json.loads(ui.describe_layout())
    check("save_first_page_prev_disabled", ui.action_at(unreal.Vector2D(180, 710)) == "")
    capture(ui, "save-populated", 2560, 1600)
    check("save_lock_existing_node", ui.execute_action("lock:" + point_id.to_string()))
    check("locked_node_delete_not_clickable", ui.action_at(unreal.Vector2D(1430, 310)) == "")
    capture(ui, "save-locked")
    check("save_unlock_existing_node", ui.execute_action("lock:" + point_id.to_string()))
    check("save_page_next", ui.execute_action("save.next"))
    check("save_last_page_next_disabled", ui.action_at(unreal.Vector2D(1460, 710)) == "")
    check("save_last_page_still_seven_rows", len(rows(ui, "ask:load:")) == 7)
    capture(ui, "save-last-page")
    ui.execute_action("save.prev")
    check("save_load_confirmation", ui.execute_action(rows(ui, "ask:load:")[0]["action"]))
    check("save_rows_blocked_by_modal", ui.action_at(unreal.Vector2D(500, 310)) == "")
    check("save_paging_blocked_by_modal", not ui.execute_action("save.next"))
    capture(ui, "save-load-confirmation")
    ui.execute_action("cancel")
    check("save_delete_confirmation", ui.execute_action(rows(ui, "ask:delete:")[0]["action"]))
    ui.execute_action("cancel")
    check("cancel_leaves_save_count", len(save.get_points()) == 10)
    for name, width, height in [("save-720p", 1280, 720), ("save-ultrawide", 2520, 1080)]:
        capture(ui, name, width, height)
    ui.execute_action("back")
    ui.execute_action("page:settings")
    for name, width, height in [("settings-16x10", 2560, 1600), ("settings-720p", 1280, 720), ("settings-ultrawide", 2520, 1080)]:
        capture(ui, name, width, height)
    ui.execute_action("category:辅助功能")
    ui.execute_action("settings.change:textscale:1")
    ui.execute_action("settings.change:textscale:1")
    check("apply_large_text", ui.execute_action("settings.apply"))
    ui.execute_action("category:键位")
    check("large_text_reduces_rows", len(rows(ui, "settings.bind:")) == 8)
    capture(ui, "settings-text-150", 2560, 1600)
    ui.execute_action("back")
    ui.execute_action("page:save")
    check("large_text_save_four_rows", len(rows(ui, "ask:load:")) == 4)
    capture(ui, "save-text-150", 2560, 1600)
    ui.execute_action("back")
    ui.execute_action("page:settings")
    ui.execute_action("category:辅助功能")
    ui.execute_action("settings.change:textscale:-1")
    ui.execute_action("settings.change:textscale:-1")
    check("restore_text_size", ui.execute_action("settings.apply"))
    ui.execute_action("back")
    check("fixture_subpage_returns_parent", str(ui.get_page()) == parent_page)
    check("populated_title_entry", ui.execute_action("page:title"))
    check("populated_title_load_entry", ui.execute_action("page:save"))
    check("title_load_caption", any(r.get("text") == "载入存档" for r in json.loads(ui.describe_layout())["components"]))
    capture(ui, "save-from-title", 2560, 1600)
    ui.execute_action("back")
    check("populated_save_returns_title", str(ui.get_page()) == "title")


def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    ui = unreal.GameplayStatics.get_player_controller(world, 0).get_hud().screen
    check("initial_title", str(ui.get_page()) == "title")
    check("default_new_game", ui.get_title_selection() == "newPrompt")
    layout = json.loads(ui.describe_layout())
    report["layout"] = layout
    check("scene_background_removed", not any(row.get("asset") == "titleBackground" for row in layout["components"]))
    top = ["continuePrompt", "newPrompt", "page:save", "page:settings"]
    bottom = ["newPrompt", "page:save", "page:settings", "ask:quit"]
    check("four_rows_initially_exit_hidden", menu(ui) == (top, {"title.next"}))
    logo = {row["id"]: row for row in layout["components"] if row["id"].startswith("title.logo.")}
    check("logo_shrunk_to_seventy_percent", abs(logo["title.logo.gui"]["rect"][2]/683 - 850/1430*0.7) < 0.001
          and abs(logo["title.logo.huo"]["rect"][2]/690 - 850/1430*0.7) < 0.001)
    check("characters_staggered_flame_between", 0 < logo["title.logo.huo"]["rect"][1] - logo["title.logo.gui"]["rect"][1] < 30
          and logo["title.logo.gui"]["rect"][0] < logo["title.logo.flame"]["rect"][0] < logo["title.logo.huo"]["rect"][0])
    for name, width, height in [("title-16x9", 1672, 941), ("title-16x10", 2560, 1600),
                                ("title-ultrawide", 2520, 1080), ("title-720p", 1280, 720)]:
        capture(ui, name, width, height)
    check("arrow_next_hit", ui.action_at(unreal.Vector2D(836, 798)) == "title.next")
    check("hidden_up_arrow_not_clickable", ui.action_at(unreal.Vector2D(836, 584)) == "")
    check("hidden_fifth_row_not_clickable", ui.action_at(unreal.Vector2D(836, 804)) == "")
    check("next_to_load", ui.execute_action("title.next") and ui.get_title_selection() == "page:save")
    check("next_to_settings", ui.execute_action("title.next") and ui.get_title_selection() == "page:settings")
    check("first_four_stay_visible_until_exit", menu(ui) == (top, {"title.next"}))
    capture(ui, "title-settings-selected")
    ui.refresh()
    check("selection_survives_refresh", ui.get_title_selection() == "page:settings")
    check("settings_entry", ui.execute_action("page:settings") and str(ui.get_page()) == "settings")
    capture(ui, "settings-entry")
    check("settings_back", ui.execute_action("back") and str(ui.get_page()) == "title")
    check("selection_survives_back", ui.get_title_selection() == "page:settings")
    check("next_to_quit", ui.execute_action("title.next") and ui.get_title_selection() == "ask:quit")
    check("exit_appears_in_fourth_row", menu(ui) == (bottom, {"title.prev"})
          and ui.action_at(unreal.Vector2D(836, 760)) == "ask:quit")
    check("arrow_prev_hit_at_bottom", ui.action_at(unreal.Vector2D(836, 584)) == "title.prev")
    check("hidden_down_arrow_not_clickable", ui.action_at(unreal.Vector2D(836, 798)) == "")
    check("bottom_clamped_without_wrap", not ui.execute_action("title.next") and ui.get_title_selection() == "ask:quit")
    report["bottom_layout"] = json.loads(ui.describe_layout())
    capture(ui, "title-bottom", 2560, 1600)
    ui.refresh()
    check("scroll_window_survives_refresh", menu(ui) == (bottom, {"title.prev"}))
    check("quit_opens_confirmation", ui.execute_action("ask:quit"))
    capture(ui, "quit-confirmation")
    check("wheel_actions_blocked_by_modal", not ui.execute_action("title.next") and ui.get_title_selection() == "ask:quit")
    check("menu_clicks_blocked_by_modal", ui.action_at(unreal.Vector2D(836, 760)) == "")
    check("cancel_quit", ui.execute_action("cancel") and str(ui.get_page()) == "title")
    check("cancel_preserves_bottom_window", menu(ui) == (bottom, {"title.prev"}))
    check("save_entry", ui.execute_action("page:save") and str(ui.get_page()) == "save")
    check("save_back", ui.execute_action("back") and str(ui.get_page()) == "title")
    check("subpage_preserves_bottom_window", menu(ui) == (bottom, {"title.prev"}))
    for expected in ["page:settings", "page:save", "newPrompt", "continuePrompt"]:
        check("previous_to_" + expected, ui.execute_action("title.prev") and ui.get_title_selection() == expected)
        check("four_rows_after_previous_" + expected, len(menu(ui)[0]) == 4)
    check("top_window_restored", menu(ui) == (top, {"title.next"}))
    check("top_clamped_without_wrap", not ui.execute_action("title.prev") and ui.get_title_selection() == "continuePrompt")
    ui.execute_action("title.next")
    check("restored_default_selection", ui.get_title_selection() == "newPrompt")
    capture(ui, "title-final")
    verify_subpages(ui, world)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.VerifyModalFocus",
                                                 unreal.GameplayStatics.get_player_controller(world, 0))
    focus_report = json.loads((out / "modal-focus.json").read_text(encoding="utf-8"))
    for name, passed in focus_report["checks"].items():
        check("modal_focus_" + name, passed)
    check("modal_focus_regression_passed", focus_report["passed"])
    report["captures"].extend(focus_report["captures"])
    ui.open_page("hud")
    # Applying settings queues an Enhanced Input rebuild; allow real game frames before testing it.
    yield delay(.3)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.VerifyHUDPreview",
                                                 unreal.GameplayStatics.get_player_controller(world, 0))
    hud_report = json.loads((out / "hud-preview.json").read_text(encoding="utf-8"))
    for name, passed in hud_report["checks"].items():
        check("hud_preview_" + name, passed)
    check("hud_preview_regression_passed", hud_report["passed"])
    report["captures"].extend(hud_report["captures"])
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.VerifyInventoryPreview",
                                                 unreal.GameplayStatics.get_player_controller(world, 0))
    inventory_report = json.loads((out / "inventory-preview.json").read_text(encoding="utf-8"))
    for name, passed in inventory_report["checks"].items():
        check("inventory_preview_" + name, passed)
    check("inventory_preview_regression_passed", inventory_report["passed"])
    report["captures"].extend(inventory_report["captures"])
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.UI.VerifySkillsPreview",
                                                 unreal.GameplayStatics.get_player_controller(world, 0))
    skills_report = json.loads((out / "skills-preview.json").read_text(encoding="utf-8"))
    for name, passed in skills_report["checks"].items():
        check("skills_preview_" + name, passed)
    check("skills_preview_regression_passed", skills_report["passed"])
    report["captures"].extend(skills_report["captures"])
    # Use actual PIE frames to complete the game's original 0.4-second equipment switch.
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    combat = pawn.get_component_by_class(unreal.HearthwardCombatComponent)
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    bag = pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    gameplay.grant_initial_equipment()  # The save-list prototype started without the natural game's loadout.
    axe_count = bag.get_item_count("axe")
    check("inventory_preview_live_fixture_owns_axe", axe_count > 0)
    ui.open_page("inventory")
    check("inventory_preview_live_axe_selected", ui.execute_action("item:axe"))
    used = ui.execute_action("use")
    report["inventory_equip_diagnostic"] = {
        "use_result": used, "combat_action": str(combat.get_editor_property("action")),
        "duration": combat.get_editor_property("duration"), "gameplay_enabled": gameplay.get_editor_property("enabled"),
        "inventory": bag.describe_inventory(), "gameplay_feedback": gameplay.get_editor_property("feedback"),
        "movement_mode": str(pawn.character_movement.get_editor_property("movement_mode")),
        "paused": unreal.GameplayStatics.is_game_paused(world)}
    check("inventory_preview_live_equip_begins_delay", used
          and report["inventory_equip_diagnostic"]["combat_action"] == "switch"
          and abs(report["inventory_equip_diagnostic"]["duration"] - .4) < .001)
    yield delay(.8)
    completed_equipment = {str(k): str(v) for k, v in gameplay.get_editor_property("equipment").items()}
    report["inventory_equip_completion"] = {
        "equipment": completed_equipment, "combat_action": str(combat.get_editor_property("action")),
        "equipped_instance": bag.equipped_instance("weapon").to_string(),
        "axe_instance": bag.first_instance("axe").to_string()}
    check("inventory_preview_live_unequip_finishes", not combat.busy()
          and "weapon" not in completed_equipment
          and report["inventory_equip_completion"]["equipped_instance"] == "0" * 32)
    # The original action toggles a currently equipped instance off; use it again to equip it.
    ui.open_page("inventory")
    ui.execute_action("item:axe")
    check("inventory_preview_live_reequip_begins_delay", ui.execute_action("use")
          and str(combat.get_editor_property("action")) == "switch")
    yield delay(.8)
    completed_equipment = {str(k): str(v) for k, v in gameplay.get_editor_property("equipment").items()}
    report["inventory_reequip_completion"] = {
        "equipment": completed_equipment, "combat_action": str(combat.get_editor_property("action")),
        "equipped_instance": bag.equipped_instance("weapon").to_string(),
        "axe_instance": bag.first_instance("axe").to_string()}
    check("inventory_preview_live_equip_finishes", not combat.busy()
          and completed_equipment.get("weapon") == "axe"
          and report["inventory_reequip_completion"]["equipped_instance"] == report["inventory_reequip_completion"]["axe_instance"])
    check("inventory_preview_live_equip_does_not_consume_axe", bag.get_item_count("axe") == axe_count)
    ui.open_page("inventory")
    check("inventory_preview_live_equipped_slot", next(r for r in json.loads(ui.describe_layout())["components"]
          if r["id"] == "inventory.equipment.weapon")["asset"] == "axe")
    capture(ui, "inventory-live-equipped")
    levels.editor_request_end_play()
    yield lambda: not levels.is_in_play_in_editor()
    check("pie_teardown", True)
    report["passed"] = True


iterator = run()
pending = None
deadline = time.monotonic() + 180


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError("Title UI PIE verification")
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
