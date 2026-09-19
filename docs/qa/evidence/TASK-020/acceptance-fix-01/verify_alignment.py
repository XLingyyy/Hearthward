"""Capture the actual HUD at several widths and headings in an isolated save pool."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{}}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>end
def run():
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check('title_neutral_capture',ui.capture_ui('fix01-title',1672,941))
    check('new_campaign',ui.execute_action('new'))
    ui.open_page('hud')
    for width,height in ((1672,941),(1280,720),(2031,1254)):
        for heading in (4,90,180,359):
            pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=heading-90,roll=0))
            actual=(pc.get_control_rotation().yaw+450)%360
            check(f'heading_{width}_{heading}',abs(actual-heading)<.01)
            ui.refresh()
            check(f'hud_{width}_{heading}',ui.capture_ui(f'fix01-hud-{width}-{heading}',width,height))
    report['passed']=True
    (out/'fix01-verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
iterator=run();pending=None;deadline=time.monotonic()+120
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('UI acceptance regression')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        (out/'fix01-verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
