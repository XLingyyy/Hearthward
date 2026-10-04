"""Run the existing native journal regression in an isolated noncampaign PIE world."""
import json
import os
from pathlib import Path
import time
import traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id = os.environ['HEARTHWARD_TITLE_RUN']
out = Path(unreal.Paths.project_saved_dir()) / 'TitleWheel' / run_id
out.mkdir(parents=True, exist_ok=True)
(Path(unreal.Paths.project_saved_dir()) / 'Task020').mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = dict(passed=False, run_id=run_id, checks={}, captures=[])
result_file = Path(os.environ['HEARTHWARD_JOURNAL_QA']) / 'report.json'

def delay(seconds):
    deadline = time.monotonic() + seconds
    return lambda: time.monotonic() >= deadline

def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    # Match the existing journal suite's adventure/prototype setup, without real player saves.
    unreal.SystemLibrary.execute_console_command(world, 'Hearthward.Companion.CreateTest', controller)
    unreal.GameplayStatics.get_player_pawn(world, 0).get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
    save = next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer() == world)
    if not save.enable_prototype() or not save.start_new_progress():
        raise AssertionError('Isolated prototype progress initialization failed')
    controller.get_hud().screen.open_page('hud')
    unreal.SystemLibrary.execute_console_command(world, 'Hearthward.UI.VerifyJournalPreview', controller)
    native = json.loads((out / 'journal-preview.json').read_text(encoding='utf-8'))
    report.update(native)
    report['native_report_path'] = str(out / 'journal-preview.json')
    report['capture_directory'] = str(Path(unreal.Paths.project_saved_dir()) / 'Task020')
    levels.editor_request_end_play()
    yield lambda: not levels.is_in_play_in_editor()
    report['checks']['pie_teardown'] = True
    report['passed'] = native['passed'] and all(report['checks'].values())

iterator = run()
pending = None
deadline = time.monotonic() + 240

def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError('Journal PIE verification timed out')
        if pending and not pending():
            return
        pending = next(iterator)
    except StopIteration:
        result_file.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['passed'] = False
        report['error'] = traceback.format_exc()
        result_file.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)

handle = unreal.register_slate_post_tick_callback(tick)
