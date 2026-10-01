"""Rendered PIE: task-local settings, fixed-cue gates, collision and natural water.

Uses native gameplay APIs and Enhanced Input injection; does not claim physical
keyboard/IME or human acceptance. Fixture meshes exist only in the PIE world.
"""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
out = root / "Saved/Task051/pie"
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report = {"ok": False, "checks": {}, "method": __doc__, "screenshots": []}
movement = None


def check(name, value):
    report["checks"][name] = bool(value)
    if not value:
        raise AssertionError(name)


def delay(seconds):
    until = time.monotonic() + seconds
    return lambda: time.monotonic() >= until


def objects():
    world = editor.get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    ui = pc.get_hud().get_editor_property("screen")
    return world, pc, pawn, ui


def shot(world, pc, name):
    unreal.SystemLibrary.execute_console_command(
        world, f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix', pc)
    report["screenshots"].append(name + ".png")
    ui = objects()[3]
    (Path(unreal.Paths.project_saved_dir()) / "Task020").mkdir(parents=True, exist_ok=True)
    check("native UI capture " + name, ui.capture_ui("TASK051-" + name, 1920, 1080))


def run():
    global movement
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(3)
    world, pc, pawn, ui = objects()
    unreal.GameplayStatics.set_game_paused(world, False)
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    gameplay.set_editor_property("enabled", True)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.Companion.CreateTest", pc)
    brother = unreal.GameplayStatics.get_actor_of_class(world, unreal.HearthwardCompanionFixture)
    check("real brother fixture", brother is not None)
    brother.set_actor_location(pawn.get_actor_location() + unreal.Vector(150, 0, 0), False, True)
    yield delay(.8)
    save = next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == world)
    check("isolated prototype save participants", save.enable_prototype())
    check("isolated progress", save.start_new_progress())
    ui.open_page("hud")
    camera = pawn.get_component_by_class(unreal.CameraComponent)
    ui.open_page("settings")
    check("defaults draft", ui.execute_action("settings.defaults"))
    check("defaults apply", ui.execute_action("settings.apply"))
    ui.execute_action("cancel")  # Display confirmation, if the mode changed.
    ui.open_page("hud")
    yield delay(.5)
    check("default FOV", abs(camera.field_of_view - 90) < .01)
    ui.open_page("settings")
    check("FOV draft adjustment", ui.execute_action("settings.change:fov:1"))
    check("draft does not change camera", abs(camera.field_of_view - 90) < .01)
    ui.open_page("hud")
    ui.open_page("settings")
    check("discarded draft leaves camera", abs(camera.field_of_view - 90) < .01)
    for key in ["fov", "textscale", "textscale", "subtitleSize", "subtitleSize"]:
        check("adjust " + key, ui.execute_action("settings.change:" + key + ":1"))
    check("apply comfort settings", ui.execute_action("settings.apply"))
    ui.open_page("hud")
    yield delay(.5)
    check("applied FOV", abs(camera.field_of_view - 95) < .01)
    import configparser
    config = configparser.ConfigParser(strict=False)
    config.read(Path(unreal.Paths.project_saved_dir()) / "Config/WindowsEditor/GameUserSettings.ini", encoding="utf-8-sig")
    report["saved_comfort"] = dict(config["Hearthward.Comfort"])
    check("comfort persisted independently", config.getint("Hearthward.Comfort", "TextScale") == 150 and config.getint("Hearthward.Comfort", "SubtitleSize") == 48)
    original_mode = unreal.GameUserSettings.get_game_user_settings().get_fullscreen_mode()
    ui.open_page("settings")
    ui.execute_action("settings.change:mode:1")
    check("display trial starts", ui.execute_action("settings.apply"))
    check("display trial differs", unreal.GameUserSettings.get_game_user_settings().get_fullscreen_mode() != original_mode)
    yield delay(16)
    check("15-second display revert while paused", unreal.GameUserSettings.get_game_user_settings().get_fullscreen_mode() == original_mode)
    check("display revert retains comfort", abs(camera.field_of_view - 95) < .01)
    ui.open_page("settings")
    ui.execute_action("category:辅助功能")
    yield delay(.5)
    shot(world, pc, "settings-150")
    yield delay(.5)
    ui.execute_action("category:键位")
    yield delay(.5)
    shot(world, pc, "bindings-150")
    yield delay(.5)
    ui.open_page("hud")
    presentation = pawn.get_component_by_class(unreal.HearthwardPresentationComponent)
    event = unreal.GuidLibrary.new_guid()
    check("unknown information suppressed", not presentation.play_fixed_cue("fixed.task.accepted", event, False))
    check("nearby observed cue", presentation.play_fixed_cue("fixed.task.accepted", event, True))
    check("missing human recording remains explicit", presentation.get_voice_status() == "UNPRODUCED")
    check("fixed subtitle displayed", bool(presentation.get_subtitle()))
    check("duplicate cue event suppressed", not presentation.play_fixed_cue("fixed.task.accepted", event, True))
    yield delay(.5)
    shot(world, pc, "subtitle-48")
    yield delay(.5)
    presentation.stop_fixed_cue()
    check("cancel stops subtitle", not presentation.get_subtitle())
    brother.set_actor_location(pawn.get_actor_location() + unreal.Vector(4000, 0, 0), False, True)
    check("distant speaker suppressed", not presentation.play_fixed_cue("fixed.task.accepted", unreal.GuidLibrary.new_guid(), True))

    # 100 cm platform with a clear, 60 cm deep landing. All dimensions are cm.
    origin = unreal.Vector(5000, 5000, 5000)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.ExperienceFixture create", pc)
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
    floor = next(a for a in actors if a.actor_has_tag("Task051VaultFloor"))
    barrier = next(a for a in actors if a.actor_has_tag("Task051VaultBarrier"))
    pawn.set_actor_location(origin + unreal.Vector(0, 0, 93), False, True)
    pc.set_control_rotation(unreal.Rotator(pitch=-10, yaw=0))
    pawn.character_movement.stop_movement_immediately()
    pawn.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FALLING)
    yield delay(.8)
    traversal = pawn.get_component_by_class(unreal.HearthwardTraversalComponent)
    gameplay.set_editor_property("stamina", 100)
    before = gameplay.stamina
    check("valid low obstacle starts vault", traversal.begin_vault())
    check("up-front stamina spent", gameplay.stamina < before)
    check("vault cannot be saved", not save.save_point(True))
    yield delay(1.3)
    check("vault completed", not traversal.is_vaulting())
    check("vault lands on platform", pawn.get_actor_location().x > origin.x + 100 and abs(pawn.get_actor_location().z - origin.z - 190) < 10)
    shot(world, pc, "vault-landed")
    yield delay(.5)
    pawn.set_actor_location(origin + unreal.Vector(0, 0, 93), False, True)
    pawn.character_movement.stop_movement_immediately()
    pawn.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FALLING)
    yield delay(.8)
    check("second vault starts", traversal.begin_vault())
    paid = gameplay.stamina
    yield delay(.15)
    gameplay.apply_damage(1)
    check("damage cancels vault", not traversal.is_vaulting())
    check("cancel retains real position", pawn.get_actor_location().x < origin.x + 100)
    check("cancel does not refund", gameplay.stamina <= paid + .1)
    yield delay(.7)
    pawn.set_actor_location(origin + unreal.Vector(0, 0, 93), False, True)
    pawn.character_movement.stop_movement_immediately()
    pawn.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FALLING)
    unreal.SystemLibrary.execute_console_command(world, "Hearthward.ExperienceFixture ceiling", pc)
    ceiling = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor) if a.actor_has_tag("Task051VaultCeiling"))
    yield delay(.8)
    check("blocked capsule path rejects vault", not traversal.begin_vault())
    ceiling.destroy_actor()
    barrier.destroy_actor()
    floor.destroy_actor()

    # Travel through the normal New Game path, then exercise the authored lake.
    ui.open_page("title")
    check("normal new-game entry", ui.execute_action("new"))
    yield lambda: editor.get_game_world() and "L_HearthwardWilds" in editor.get_game_world().get_name() and unreal.GameplayStatics.get_player_controller(editor.get_game_world(), 0)
    world, pc, pawn, ui = objects()
    loading = next(x for x in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem) if x.get_outer() == unreal.GameplayStatics.get_game_instance(world))
    yield lambda: not loading.is_loading()
    check("comfort survives map travel", abs(pawn.get_component_by_class(unreal.CameraComponent).field_of_view - 95) < .01)
    pawn.set_actor_location(unreal.Vector(-93000, -36000, 15680), False, True)
    pc.set_control_rotation(unreal.Rotator(pitch=-5, yaw=0))
    pawn.character_movement.stop_movement_immediately()
    traversal = pawn.get_component_by_class(unreal.HearthwardTraversalComponent)
    gameplay = pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    gameplay.set_editor_property("stamina", 100)
    yield delay(3)
    check("natural lake selects swimming", traversal.is_in_water() and pawn.character_movement.is_swimming())
    check("swim speed is 300 cm/s", pawn.character_movement.max_swim_speed == 300)
    before = gameplay.stamina
    yield delay(2)
    check("floating has no stamina fee", abs(gameplay.stamina - before) < .05)
    subsystem = next(x for x in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(x.get_outer(), unreal.LocalPlayer))
    action = next(a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer() == pawn and "AXIS2D" in str(a.value_type) and any(str(unreal.InputLibrary.key_get_display_name(k)) == "W" for k in subsystem.query_keys_mapped_to_action(a)))
    start = pawn.get_actor_location()
    movement = (subsystem, action)
    yield delay(2)
    movement = None
    check("swimming input moves", (pawn.get_actor_location() - start).length() > 200)
    check("paddling spends stamina once", before - gameplay.stamina > 4 and before - gameplay.stamina < 15)
    shot(world, pc, "natural-swim")
    yield delay(.5)
    gameplay.set_editor_property("stamina", 0)
    sunk_from = pawn.get_actor_location().z
    yield delay(1.1)
    check("zero stamina sinks within 50 cm per second", 35 < sunk_from - pawn.get_actor_location().z < 70)
    yield delay(10)
    check("natural drowning is true death", pawn.get_component_by_class(unreal.HearthwardSurvivalComponent).state.life == unreal.HearthwardLife.DEAD)
    check("drowning opens recoverable failure page", str(ui.get_page()) == "save")
    report["ok"] = True


runner = run()
pending = None
deadline = time.monotonic() + 480


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError("TASK-051 runtime verification")
        if movement:
            movement[0].inject_input_vector_for_action(movement[1], unreal.Vector(0, 1, 0), [], [])
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
