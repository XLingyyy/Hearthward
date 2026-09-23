"""TASK-042 player route through normal world movement and UI actions; no item grants or teleports."""
import json
import math
import re
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / "Task029Integration"
out.mkdir(parents=True, exist_ok=True)
report = {"passed": False, "checks": {}, "scope": "PIE movement and UI actions, not physical keys or final art"}
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def wait(predicate, seconds=45):
    return predicate, time.monotonic() + seconds


def delay(seconds):
    deadline = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= deadline)


def game():
    return editor.get_game_world()


def screen():
    pc = unreal.GameplayStatics.get_player_controller(game(), 0)
    return pc.get_hud().screen if pc and pc.get_hud() else None


def move_toward(pawn, target, distance=100):
    point = target.get_actor_location()
    current = pawn.get_actor_location()
    dx, dy = point.x - current.x, point.y - current.y
    remaining = math.hypot(dx, dy)
    if remaining <= distance:
        return True
    pawn.add_movement_input(unreal.Vector(dx / remaining, dy / remaining, 0), 1)
    return False


def run():
    check("isolated_pool", re.search(r"-HearthwardSaveTestPool=[0-9a-fA-F-]+", unreal.SystemLibrary.get_command_line()))
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(3)
    check("natural_entry", screen().execute_action("new"))
    yield wait(lambda: game() and str(unreal.GameplayStatics.get_current_level_name(game(), True)) == "L_HearthwardWilds"
               and screen() is not None and str(screen().get_page()) == "hud", 120)
    pawn = unreal.GameplayStatics.get_player_pawn(game(), 0)
    companion = unreal.GameplayStatics.get_all_actors_of_class(game(), unreal.HearthwardCompanionFixture)[0]
    check("wood_bound_to_authored_tree", companion.source.get_owner().actor_has_tag(
        "Hearthward.NaturalCamp.AuthoredTree.PROTOTYPE_ONLY"))
    source, camp = companion.source.get_owner(), companion.camp
    bag = pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    interaction = pawn.get_component_by_class(unreal.HearthwardInteractionComponent)
    building = pawn.get_component_by_class(unreal.HearthwardBuildingComponent)

    yield wait(lambda: move_toward(pawn, source, 95), 30)
    yield delay(.5)
    check("reached_resource_by_movement", move_toward(pawn, source, 110))
    for index in range(11):
        before = bag.get_item_count("wood")
        check("gather_started_%02d" % index, interaction.interact_nearest())
        yield wait(lambda: bag.get_item_count("wood") == before + 1, 9)
        check("gathered_%02d" % index, bag.get_item_count("wood") == before + 1)
        yield delay(.15)

    check("inventory_entry", screen().execute_action("page:inventory") and str(screen().get_page()) == "inventory")
    check("inventory_wood", bag.get_item_count("wood") == 11)
    check("inventory_back", screen().execute_action("back") and str(screen().get_page()) == "hud")
    check("building_entry", screen().execute_action("page:building"))
    check("workbench_selected", screen().execute_action("build:workbench"))
    yield delay(.6)
    check("outside_camp_placement_refused", not building.valid_placement and "营地" in str(building.feedback))
    yield wait(lambda: move_toward(pawn, camp, 85), 35)
    yield delay(.6)
    check("returned_to_camp_for_building", move_toward(pawn, camp, 100))
    report["placement_feedback"] = str(building.feedback)
    check("natural_placement_valid", building.valid_placement)
    check("build_started", building.confirm_placement())
    yield wait(lambda: building.building_count() == 1, 9)
    check("workbench_built", building.building_count() == 1 and bag.get_item_count("wood") == 3)
    check("crafting_entry", screen().execute_action("page:crafting"))
    before_arrow = bag.get_item_count("arrow")
    check("arrow_recipe", screen().execute_action("recipe:arrows"))
    check("arrows_crafted", screen().execute_action("craft") and bag.get_item_count("arrow") == before_arrow + 4)
    check("crafting_back", screen().execute_action("back") and str(screen().get_page()) == "hud")

    yield wait(lambda: move_toward(pawn, camp, 85), 30)
    yield delay(.5)
    check("reached_camp_by_movement", move_toward(pawn, camp, 100))
    check("storage_entry", screen().execute_action("page:storage"))
    check("wood_selected_for_deposit", screen().execute_action("deposit:wood"))
    before_wood = bag.get_item_count("wood")
    check("storage_transfer", screen().execute_action("transfer") and bag.get_item_count("wood") == before_wood - 1)
    check("storage_back", screen().execute_action("back") and str(screen().get_page()) == "hud")
    check("journal_entry", screen().execute_action("page:journal"))
    check("map_from_journal", screen().execute_action("page:map"))
    check("return_to_journal", screen().execute_action("back") and str(screen().get_page()) == "journal")
    check("return_to_world", screen().execute_action("back") and str(screen().get_page()) == "hud")
    check("saved", screen().execute_action("save"))
    report["passed"] = all(report["checks"].values())
    (out / "natural-player-route.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    levels.editor_request_end_play()


iterator = run()
pending = None


def tick(_delta):
    global pending
    try:
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("TASK-042 player route wait")
                return
        pending = next(iterator)
    except StopIteration:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    except Exception:
        report["error"] = traceback.format_exc()
        (out / "natural-player-route.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        unreal.log_error(report["error"])
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
