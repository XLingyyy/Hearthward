"""Reuse existing native HUD checks, then capture the updated rails in an isolated real world."""
import json
import os
from pathlib import Path
import time
import traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id = os.environ['HEARTHWARD_HUD_RUN']
title_run = os.environ['HEARTHWARD_TITLE_RUN']
saved = Path(unreal.Paths.project_saved_dir())
out = saved / 'HUDPreview' / run_id
out.mkdir(parents=True, exist_ok=True)
(saved / 'Task020').mkdir(parents=True, exist_ok=True)
(saved / 'TitleWheel' / title_run).mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report = {'run_id': run_id, 'passed': False, 'checks': {}, 'captures': [], 'ui_captures': [], 'layouts': []}


def check(name, value):
    report['checks'][name] = bool(value)
    if not value:
        raise AssertionError(name)


def delay(seconds):
    end = time.monotonic() + seconds
    return lambda: time.monotonic() >= end


def natural_ready():
    world = editor.get_game_world()
    if not world or unreal.GameplayStatics.get_current_level_name(world, True) != 'L_HearthwardWilds':
        return False
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    loading = next((s for s in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem)
                    if s.get_outer() == unreal.GameplayStatics.get_game_instance(world)), None)
    return pc and pc.get_hud() and pc.get_hud().screen and loading and not loading.is_loading()


def capture_scene(world, ui, name):
    ui.refresh()
    report['layouts'].append(json.loads(ui.describe_layout()))
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    unreal.SystemLibrary.execute_console_command(world, 'Hearthward.UI.CaptureHUDScene ' + name, pc)
    check('capture_' + name, (out / (name + '.png')).is_file())
    report['captures'].append(name + '.png')


def capture_ui(ui, name):
    filename = run_id + '-' + name
    check('capture_' + name, ui.capture_ui(filename, 1920, 1080))
    report['layouts'].append(json.loads(ui.describe_layout()))
    report['ui_captures'].append(filename + '.png')


def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world = editor.get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    # The existing native suite expects the same explicit companion/adventure fixture as verify_title.py.
    unreal.SystemLibrary.execute_console_command(world, 'Hearthward.Companion.CreateTest', pc)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pawn.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
    save = next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer() == world)
    check('isolated_native_fixture_enabled', save.enable_prototype())
    check('isolated_native_progress_created', save.start_new_progress())
    unreal.SystemLibrary.execute_console_command(world, 'Hearthward.UI.VerifyHUDPreview', pc)
    native_path = saved / 'TitleWheel' / title_run / 'hud-preview.json'
    check('native_hud_report_exists', native_path.is_file())
    native = json.loads(native_path.read_text(encoding='utf-8'))
    (out / 'hud-preview.json').write_text(json.dumps(native, ensure_ascii=False, indent=2), encoding='utf-8')
    report['checks'].update({'native_' + k: v for k, v in native['checks'].items()})
    report['ui_captures'].extend(native['captures'])
    check('existing_native_hud_suite', native['passed'])
    ui = pc.get_hud().screen
    check('isolated_new_game', ui.execute_action('new'))
    yield natural_ready
    world = editor.get_game_world()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    ui = pc.get_hud().screen
    check('natural_world_hud', str(ui.get_page()) == 'hud')
    report['map'] = unreal.GameplayStatics.get_current_level_name(world, True)
    yield delay(.5)
    capture_scene(world, ui, 'hud-gloss-full')
    gameplay = unreal.GameplayStatics.get_player_pawn(world, 0).get_component_by_class(unreal.HearthwardGameplayComponent)
    original = {key: gameplay.get_editor_property(key) for key in ('health', 'hunger', 'stamina')}
    try:
        gameplay.set_editor_property('health', gameplay.max_health() * .72)
        gameplay.set_editor_property('hunger', 55.)
        gameplay.set_editor_property('stamina', gameplay.max_stamina() * .82)
        ui.refresh()
        yield delay(.2)
        capture_scene(world, ui, 'hud-gloss-partial')
        capture_ui(ui, 'hud-gloss-partial-16x9')
        # Capture the almost-empty channels synchronously, then restore the disposable fixture immediately.
        for key in original:
            gameplay.set_editor_property(key, 1. if key == 'health' else 0.)
        capture_ui(ui, 'hud-gloss-empty-16x9')
    finally:
        for key, value in original.items():
            gameplay.set_editor_property(key, value)
        ui.refresh()
    levels.editor_request_end_play()
    yield lambda: not levels.is_in_play_in_editor()
    report['passed'] = all(report['checks'].values())


iterator = run()
pending = None
deadline = time.monotonic() + 240


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError('HUD gloss preview')
        if pending and not pending():
            return
        pending = next(iterator)
    except StopIteration:
        (out / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error'] = traceback.format_exc()
        (out / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)


handle = unreal.register_slate_post_tick_callback(tick)
