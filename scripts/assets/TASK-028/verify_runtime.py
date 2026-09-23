"""PIE checks for the TASK-028 house and natural-map facility wiring.

The launch wrapper supplies an isolated HearthwardSaveTestPool ID. Wood is
injected only inside PIE so construction can be tested without changing a save.
"""

import json
import time
import traceback
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / "Saved/Task028/runtime-qa.json"
MAP = "/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"
BOOTSTRAP = "/Game/Hearthward/Bootstrap/L_Bootstrap"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
report = {"ok": False, "map": MAP, "checks": {},
          "test_only_material_injection": {"wood": 20, "stone": 4, "axe": 1}}
state = {}


def check(name, condition, detail=None):
    report["checks"][name] = bool(condition)
    if detail is not None:
        report.setdefault("details", {})[name] = detail
    if not condition:
        raise AssertionError(f"{name}: {detail}")


def wait_for(predicate, seconds=60):
    return predicate, time.monotonic() + seconds


def sleep_stage(seconds):
    end = time.monotonic() + seconds
    return wait_for(lambda: time.monotonic() >= end, seconds + 15)


def one_component(actor, cls):
    return actor.get_component_by_class(cls)


def facility_meshes(actor):
    return [m.get_path_name() for c in actor.get_components_by_class(unreal.StaticMeshComponent)
            if (m := c.get_editor_property("static_mesh"))]


def flow():
    check("bootstrap_reopens", levels.load_level(BOOTSTRAP))
    levels.editor_request_begin_play()
    yield wait_for(levels.is_in_play_in_editor, 90)
    world = editor.get_game_world()
    yield wait_for(lambda: unreal.GameplayStatics.get_player_pawn(world, 0), 90)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    hud = pc.get_hud()
    screen = hud.get_editor_property("screen")
    check("bootstrap_title_screen", bool(screen) and str(screen.get_page()) == "title",
          {"hud": hud.get_class().get_name(), "page": str(screen.get_page()) if screen else None})
    check("normal_new_game_entry", screen.execute_action("new"), screen.get_message())
    yield wait_for(lambda: editor.get_game_world() and
                   unreal.GameplayStatics.get_current_level_name(editor.get_game_world(), True) == "L_HearthwardWilds", 120)
    world = editor.get_game_world()
    yield wait_for(lambda: unreal.GameplayStatics.get_player_pawn(world, 0), 90)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    state.update(world=world, pawn=pawn, pc=pc)
    report["runtime_classes"] = {
        "map": unreal.GameplayStatics.get_current_level_name(world, True),
        "game_mode": unreal.GameplayStatics.get_game_mode(world).get_class().get_name(),
        "pawn": pawn.get_class().get_name(),
    }
    yield sleep_stage(2)

    houses = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardTask028CampHouse)
    check("one_camp_house", len(houses) == 1, len(houses))
    house = houses[0]
    p = house.get_actor_location()
    report["house_cm"] = [round(p.x, 2), round(p.y, 2), round(p.z, 2)]
    check("house_near_camp", abs(p.x + 98600) < 1 and abs(p.y + 75500) < 1 and 15900 < p.z < 16300)
    meshes = facility_meshes(house)
    report["house_visuals"] = meshes
    check("house_has_29_visual_parts", len(meshes) == 29, len(meshes))
    check("all_eight_house_modules_present", all("/house/" + name + "/SM_" + name in " ".join(meshes)
         for name in ("wood_floor_panel", "wood_stairs", "wood_wall_solid", "wood_wall_doorway",
                      "wood_wall_window", "wood_door", "thatched_roof_slope", "thatched_roof_ridge")))
    boxes = house.get_components_by_class(unreal.BoxComponent)
    report["house_collision_boxes"] = [b.get_name() for b in boxes]
    check("house_walkable_collision", len(boxes) == 11, len(boxes))
    hud = pc.get_hud()
    screen = hud.get_editor_property("screen")
    check("natural_screen_exists", bool(screen), hud.get_class().get_name())
    save = next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer() == world)
    gameplay = one_component(pawn, unreal.HearthwardGameplayComponent)
    inventory = one_component(pawn, unreal.HearthwardInventoryComponent)
    building = one_component(pawn, unreal.HearthwardBuildingComponent)
    check("natural_gameplay_enabled", save.is_natural_world_enabled() and gameplay.enabled)
    check("old_fixture_landmarks_absent", not unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Companion.PROTOTYPE_ONLY"))
    check("building_page_available", screen.execute_action("page:building") and str(screen.get_page()) == "building", str(screen.get_page()))
    screen.execute_action("page:hud")
    inventory.try_add("wood", 20)
    inventory.try_add("stone", 4)
    check("test_wood_in_bag", inventory.get_item_count("wood") == 20, inventory.get_item_count("wood"))
    check("test_stone_in_bag", inventory.get_item_count("stone") == 4, inventory.get_item_count("stone"))
    check("campfire_selected", screen.execute_action("build:campfire") and building.is_placing(), building.feedback)
    yield sleep_stage(2)
    previews = unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Building.Preview")
    check("campfire_preview_uses_real_mesh", len(previews) == 1 and
          any("/Assets/Demo/campfire/campfire_model" in x for x in facility_meshes(previews[0])),
          [facility_meshes(x) for x in previews])
    check("campfire_placement_valid", building.valid_placement, building.feedback)
    check("campfire_confirmation", building.confirm_placement(), building.feedback)
    yield wait_for(lambda: building.building_count() == 1, 25)
    check("campfire_consumed_exact_materials", inventory.get_item_count("wood") == 16,
          inventory.get_item_count("wood"))
    built = building.get_buildings()
    check("completed_campfire_uses_real_mesh", len(built) == 1 and
          any("/Assets/Demo/campfire/campfire_model" in x for x in facility_meshes(built[0])),
          [facility_meshes(x) for x in built])
    pc.set_control_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0))
    check("workbench_selected", screen.execute_action("build:workbench") and building.is_placing(), building.feedback)
    yield sleep_stage(2)
    if not building.valid_placement:
        camp = p + unreal.Vector(600, 500, 0)
        for dx, dy, yaw in ((500, 500, 0), (500, 500, 90), (0, 650, 180),
                            (650, 0, 270), (350, -600, 45), (-300, 650, 135)):
            building.cancel_placement()
            pawn.set_actor_location(camp + unreal.Vector(dx, dy, 100), False, True)
            pc.set_control_rotation(unreal.Rotator(pitch=-20, yaw=yaw, roll=0))
            screen.execute_action("build:workbench")
            yield sleep_stage(.7)
            if building.valid_placement:
                report["workbench_placement_setup"] = {"offset": [dx, dy, 100], "yaw": yaw}
                break
    previews = unreal.GameplayStatics.get_all_actors_with_tag(world, "Hearthward.Building.Preview")
    check("workbench_preview_uses_real_mesh", len(previews) == 1 and
          any("/Assets/Demo/table/wood_table_model" in x for x in facility_meshes(previews[0])),
          [facility_meshes(x) for x in previews])
    check("workbench_placement_valid", building.valid_placement, building.feedback)
    check("workbench_confirmation", building.confirm_placement(), building.feedback)
    yield wait_for(lambda: building.building_count() == 2, 25)
    check("workbench_consumed_exact_materials", inventory.get_item_count("wood") == 8,
          inventory.get_item_count("wood"))
    check("completed_workbench_uses_real_mesh", any("/Assets/Demo/table/wood_table_model" in x
          for x in facility_meshes(building.get_buildings()[1])))
    check("workbench_crafting_page", screen.execute_action("page:crafting") and
          str(screen.get_page()) == "crafting", str(screen.get_page()))
    screen.execute_action("page:hud")

    if inventory.get_item_count("axe") == 0:
        inventory.try_add("axe", 1)
        report["test_only_material_injection"]["axe"] = 1
    else:
        report["test_only_material_injection"]["axe"] = 0
    check("axe_in_bag", inventory.get_item_count("axe") == 1, inventory.get_item_count("axe"))
    held = next(c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == "HeldAxe")
    check("axe_mesh_bound", "/TASK-028/props/stone_bone_axe/SM_stone_bone_axe" in
          held.get_editor_property("static_mesh").get_path_name())
    if held.get_editor_property("visible"):
        check("initial_axe_unequip", gameplay.equip("axe") and not held.get_editor_property("visible"))
    check("axe_equip_shows_mesh", gameplay.equip("axe") and held.get_editor_property("visible"))
    check("axe_unequip_hides_mesh", gameplay.equip("axe") and not held.get_editor_property("visible"))
    check("axe_reequip_shows_mesh", gameplay.equip("axe") and held.get_editor_property("visible"))
    check("manual_save", save.save_point(True), save.get_status())
    points = save.get_points()
    saved_id = points[-1].save_id
    report["isolated_save_points"] = len(points)
    check("saved_point_reloads", save.load_point(saved_id), save.get_status())
    check("facilities_restored", building.building_count() == 2 and
          any("/Assets/Demo/campfire/campfire_model" in x for x in facility_meshes(building.get_buildings()[0])))
    check("workbench_mesh_restored", any("/Assets/Demo/table/wood_table_model" in x
          for x in facility_meshes(building.get_buildings()[1])))
    check("materials_restored", inventory.get_item_count("wood") == 8, inventory.get_item_count("wood"))
    check("held_axe_restored", held.get_editor_property("visible") and inventory.get_item_count("axe") == 1)

    # Walking uses the existing Enhanced Input move action. Only the test setup teleports.
    report["house_walk_setup_teleport"] = "outside entry stairs; movement itself uses Enhanced Input"
    actions = sorted([a for a in unreal.ObjectIterator(unreal.InputAction)
                      if a.get_outer() == pawn and a.value_type == unreal.InputActionValueType.AXIS2D],
                     key=lambda a: a.get_name())
    check("native_move_action_available", len(actions) == 2, len(actions))
    input_systems = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                     if isinstance(s.get_outer(), unreal.LocalPlayer) and s.query_keys_mapped_to_action(actions[0])]
    check("single_move_input_owner", len(input_systems) == 1, len(input_systems))
    state.update(input=input_systems[0], action=actions[0])
    entry = house.get_actor_location()
    pawn.character_movement.stop_movement_immediately()
    pawn.set_actor_location(unreal.Vector(entry.x + 480, entry.y - 136, entry.z + 100), False, True)
    yield sleep_stage(1)
    pc.set_control_rotation(unreal.Rotator(pitch=0, yaw=180, roll=0))
    state["moving"] = True
    yield wait_for(lambda: pawn.get_actor_location().x < entry.x + 160, 15)
    state["moving"] = False
    entered = pawn.get_actor_location()
    report["house_entry_cm"] = [round(entered.x, 2), round(entered.y, 2), round(entered.z, 2)]
    check("walked_through_house_door", abs(pawn.get_actor_location().y - (entry.y - 136)) < 80)
    pawn.character_movement.stop_movement_immediately()
    pc.set_control_rotation(unreal.Rotator(pitch=0, yaw=0, roll=0))
    state["moving"] = True
    yield wait_for(lambda: pawn.get_actor_location().x > entry.x + 465, 15)
    state["moving"] = False
    exited = pawn.get_actor_location()
    report["house_exit_cm"] = [round(exited.x, 2), round(exited.y, 2), round(exited.z, 2)]
    check("walked_out_of_house", pawn.character_movement.is_moving_on_ground())
    state.pop("input", None)

    check("return_to_title", screen.execute_action("title"))
    yield wait_for(lambda: editor.get_game_world() and
                   unreal.GameplayStatics.get_current_level_name(editor.get_game_world(), True) == "L_Bootstrap", 120)
    title_world = editor.get_game_world()
    title_pc = unreal.GameplayStatics.get_player_controller(title_world, 0)
    title_screen = title_pc.get_hud().get_editor_property("screen")
    check("title_ready_for_continue", str(title_screen.get_page()) == "title")
    check("normal_continue_entry", title_screen.execute_action("continue"))
    yield wait_for(lambda: editor.get_game_world() and
                   unreal.GameplayStatics.get_current_level_name(editor.get_game_world(), True) == "L_HearthwardWilds", 120)
    continued_world = editor.get_game_world()
    continued_pawn = unreal.GameplayStatics.get_player_pawn(continued_world, 0)
    continued_building = one_component(continued_pawn, unreal.HearthwardBuildingComponent)
    continued_bag = one_component(continued_pawn, unreal.HearthwardInventoryComponent)
    continued_axe = next(c for c in continued_pawn.get_components_by_class(unreal.StaticMeshComponent)
                         if c.get_name() == "HeldAxe")
    check("continue_restores_both_facilities", continued_building.building_count() == 2)
    check("continue_restores_materials", continued_bag.get_item_count("wood") == 8)
    check("continue_restores_held_axe", continued_axe.get_editor_property("visible"))
    check("continue_spawns_one_house", len(unreal.GameplayStatics.get_all_actors_of_class(
          continued_world, unreal.HearthwardTask028CampHouse)) == 1)
    levels.editor_request_end_play()
    yield wait_for(lambda: not levels.is_in_play_in_editor(), 30)
    report["ok"] = True


def finish(error=None):
    if error:
        report["error"] = error
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    unreal.unregister_slate_post_tick_callback(handle)


steps = flow()
pending = None


def tick(_delta):
    global pending
    try:
        if state.get("moving"):
            state["input"].inject_input_vector_for_action(state["action"], unreal.Vector(0, 1, 0), [], [])
        if pending:
            predicate, deadline = pending
            if not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError("PIE stage timeout")
                return
        pending = next(steps)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
