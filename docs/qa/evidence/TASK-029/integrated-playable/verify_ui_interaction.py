"""TASK-041 PIE page transitions and authoritative inventory transaction."""
import json
import re
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task029Integration"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"passed": False, "checks": {}, "scope": "PIE handlers and gameplay state; physical keyboard input not covered"}


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=45):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    deadline = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= deadline)


def page(ui):
    return str(ui.get_page())


def current_ui():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0) if world else None
    hud = pc.get_hud() if pc else None
    return hud.screen if hud else None


def run():
    check("isolated_pool", re.search(r"-HearthwardSaveTestPool=[0-9a-fA-F-]+", unreal.SystemLibrary.get_command_line()))
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(3)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("game_world", world is not None)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    hud = pc.get_hud()
    ui = hud.screen
    bag = pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    storage = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer() == world)
    save = next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == world)
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)

    check("title", page(ui) == "title")
    check("new_campaign_requested", ui.execute_action("new"))
    yield wait(lambda: current_ui() and page(current_ui()) == "hud"
               and str(unreal.GameplayStatics.get_current_level_name(
                   unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(), True)) == "L_HearthwardWilds", 120)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    hud = pc.get_hud()
    ui = hud.screen
    bag = pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    storage = next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer() == world)
    save = next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == world)
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    check("new_campaign", page(ui) == "hud")
    check("world_running", not unreal.GameplayStatics.is_game_paused(world))
    hud.toggle_inventory()
    check("inventory_entry", page(ui) == "inventory" and unreal.GameplayStatics.is_game_paused(world))
    check("inventory_select", ui.execute_action("item:axe"))
    check("real_equip", ui.execute_action("use") and str(gameplay.equipment.get("weapon")) == "axe")
    check("inventory_back", ui.execute_action("back") and page(ui) == "hud" and not unreal.GameplayStatics.is_game_paused(world))

    hud.open_skills()
    check("skills_entry", page(ui) == "skills")
    check("skill_select", ui.execute_action("skill:strong"))
    check("real_skill_learn", ui.execute_action("learn") and gameplay.skills.get(unreal.Name("strong")) == 1)
    check("skills_back", ui.execute_action("back") and page(ui) == "hud")

    hud.open_journal()
    check("journal_entry", page(ui) == "journal")
    check("quest_select", ui.execute_action("quest:ember"))
    check("real_quest_untrack", ui.execute_action("track") and str(gameplay.tracked_quest) == "None")
    check("real_quest_retrack", ui.execute_action("track") and str(gameplay.tracked_quest) == "ember")
    check("quest_map", ui.execute_action("questMap") and page(ui) == "map")
    check("map_settings_entry", ui.execute_action("page:settings") and page(ui) == "settings")
    check("settings_back_to_map", ui.execute_action("back") and page(ui) == "map")
    check("map_back_to_journal", ui.execute_action("back") and page(ui) == "journal")
    check("journal_back", ui.execute_action("back") and page(ui) == "hud")
    hud.open_journal()
    check("side_category", ui.execute_action("category:side") and ui.get_category() == "side")
    check("side_to_map", ui.execute_action("page:map") and page(ui) == "map")
    check("side_back_category", ui.execute_action("back") and page(ui) == "journal" and ui.get_category() == "side")
    check("side_back_world", ui.execute_action("back") and page(ui) == "hud")
    hud.open_map()
    check("direct_map_entry", page(ui) == "map")
    check("direct_map_back", ui.execute_action("back") and page(ui) == "hud")

    companion = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardCompanionFixture)[0]
    pos = companion.get_actor_location()
    pawn.character_movement.stop_movement_immediately()
    pawn.set_actor_location(unreal.Vector(pos.x - 100, pos.y, pos.z), False, True)
    hud.toggle_dialogue()
    check("dialogue_entry", page(ui) == "dialogue" and not unreal.GameplayStatics.is_game_paused(world))
    check("memory_entry", ui.execute_action("page:memory") and page(ui) == "memory" and unreal.GameplayStatics.is_game_paused(world))
    draft = next(x for x in unreal.ObjectIterator(unreal.EditableTextBox) if x.get_outer().get_outer() == ui)
    draft.set_text("尚未提交的记忆草稿")
    check("memory_draft_entered", str(draft.get_text()) == "尚未提交的记忆草稿")
    check("memory_settings_entry", ui.execute_action("page:settings") and page(ui) == "settings")
    check("settings_back_to_memory", ui.execute_action("back") and page(ui) == "memory")
    check("memory_draft_restored", str(draft.get_text()) == "尚未提交的记忆草稿")
    check("memory_back", ui.execute_action("back") and page(ui) == "dialogue" and not unreal.GameplayStatics.is_game_paused(world))
    check("dialogue_back", ui.execute_action("back") and page(ui) == "hud")
    hud.toggle_dialogue()
    check("dialogue_key_handler_reentry", page(ui) == "dialogue")
    hud.toggle_dialogue()
    check("dialogue_key_handler_close", page(ui) == "hud")

    camp = companion.camp.get_actor_location()
    pawn.set_actor_location(unreal.Vector(camp.x - 100, camp.y, camp.z), False, True)
    check("storage_entry", ui.execute_action("page:storage") and page(ui) == "storage")
    check("storage_pauses", unreal.GameplayStatics.is_game_paused(world))
    before_grant = bag.get_item_count("wood")
    bag.try_add("wood", 2)
    check("fixture_wood", bag.get_item_count("wood") == before_grant + 2)
    before_bag, before_store = bag.get_item_count("wood"), storage.get_item_count("wood")
    check("select_deposit", ui.execute_action("deposit:wood"))
    check("real_transfer", ui.execute_action("transfer") and bag.get_item_count("wood") == before_bag - 1
          and storage.get_item_count("wood") == before_store + 1)
    check("storage_back", ui.execute_action("back") and page(ui) == "hud" and not unreal.GameplayStatics.is_game_paused(world))
    hud.toggle_storage_menu()
    check("storage_key_handler_reentry", page(ui) == "storage")
    hud.toggle_storage_menu()
    check("storage_key_handler_close", page(ui) == "hud")

    hud.toggle_save_menu()
    check("save_entry", page(ui) == "save" and unreal.GameplayStatics.is_game_paused(world))
    check("real_save", ui.execute_action("save") and len(save.get_points()) > 0)
    saved_point = save.get_points()[-1]
    saved_wood = bag.get_item_count("wood")
    bag.try_add("wood", 1)
    check("post_save_mutation", bag.get_item_count("wood") == saved_wood + 1)
    check("real_load", ui.execute_action("load:" + saved_point.save_id.to_string())
          and bag.get_item_count("wood") == saved_wood and page(ui) == "hud")
    hud.toggle_save_menu()
    check("save_back", ui.execute_action("back") and page(ui) == "hud")
    hud.toggle_save_menu()
    check("save_key_handler_reentry", page(ui) == "save")
    hud.toggle_save_menu()
    check("save_key_handler_close", page(ui) == "hud")
    hud.open_pause()
    check("pause_entry", page(ui) == "pause" and unreal.GameplayStatics.is_game_paused(world))
    check("settings_entry", ui.execute_action("page:settings") and page(ui) == "settings")
    check("settings_back", ui.execute_action("back") and page(ui) == "pause")
    check("modal_entry", ui.execute_action("ask:title"))
    check("modal_blocks_navigation", not ui.execute_action("page:inventory") and page(ui) == "pause")
    hud.toggle_inventory()
    hud.open_map()
    hud.open_skills()
    hud.open_journal()
    hud.toggle_save_menu()
    hud.open_pause()
    check("modal_blocks_hud_handlers", page(ui) == "pause" and unreal.GameplayStatics.is_game_paused(world))
    check("modal_cancel", ui.execute_action("cancel") and page(ui) == "pause")
    check("pause_back", ui.execute_action("back") and page(ui) == "hud" and not unreal.GameplayStatics.is_game_paused(world))
    report["passed"] = all(report["checks"].values())
    (out / "verification.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")


iterator = run()
pending = None


def tick(_delta):
    global pending
    try:
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("PIE UI wait")
                return
        pending = next(iterator)
    except StopIteration:
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()
    except Exception:
        report["error"] = traceback.format_exc()
        (out / "verification.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        unreal.log_error(report["error"])
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
