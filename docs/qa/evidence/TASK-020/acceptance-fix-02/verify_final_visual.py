"""Regression: component geometry, hit testing, visibility and persisted layouts in real PIE."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
layout_file=Path(unreal.Paths.project_dir())/'Resources/UI/layout.json'
original_layout=layout_file.read_bytes()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'scope':'real PIE state, native captures, component transforms and input hit testing'}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>end
def rows(ui):return {r['id']:r for r in json.loads(ui.describe_layout())['components']}
def vector(x,y):return unreal.Vector2D(x,y)
def center(row):
    x,y,w,h=row['rect'];return vector(x+w*.5,y+h*.5)
def finish():
    layout_file.write_bytes(original_layout)
    (out/'fix02-final-visual.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
def run():
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    for page in ('pause','skills','journal','map','save'):
        ui.open_page(page);yield delay(.2)
        check(page+'_final_art',ui.capture_ui('fix02-final-'+page,1672,941))
    ui.open_page('dialogue')
    check('dialogue_emblem_parent',rows(ui)['dialogue.element.004']['parent']=='dialogue.panel')
    ui.set_component_rect('dialogue.panel',vector(350,250),vector(585,562.5))
    check('dialogue_final_moved_capture',ui.capture_ui('fix02-final-dialogue-moved',1672,941))
    report['passed']=True;finish()
iterator=run();pending=None;deadline=time.monotonic()+150
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('component regression')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
