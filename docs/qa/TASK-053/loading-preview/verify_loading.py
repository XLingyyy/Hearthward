"""Render the actual loading Slate widget and observe an isolated new-game transition."""
import json,os,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id=os.environ['HEARTHWARD_HUD_RUN']
out=Path(unreal.Paths.project_saved_dir())/'HUDPreview'/run_id
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report=dict(passed=False,checks={},captures=[])
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end
def loading(world):
    gi=unreal.GameplayStatics.get_game_instance(world)
    return next((s for s in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem) if s.get_outer()==gi),None)
def natural_world():
    world=editor.get_game_world()
    return world if world and unreal.GameplayStatics.get_current_level_name(world,True)=='L_HearthwardWilds' else None
def capture_scene(name):
    world=editor.get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.UI.CaptureHUDScene '+name,pc)
    check('viewport_capture_'+name,(out/(name+'.png')).is_file())
    report['captures'].append(name+'.png')
def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
    for name,w,h in [('loading-16x9',1920,1080),('loading-16x10',1600,1000),('loading-720p',1280,720),('loading-ultrawide',2560,1080)]:
        unreal.SystemLibrary.execute_console_command(world,f'Hearthward.UI.CaptureLoading {name} {w} {h}',pc)
        check('render_'+name,(out/(name+'.png')).is_file())
        report['captures'].append(name+'.png')
    check('new_game',pc.get_hud().screen.execute_action('new'))
    yield natural_world
    world=natural_world()
    system=loading(world)
    check('real_transition_loading_visible',system and system.is_loading())
    capture_scene('loading-real-transition')
    yield lambda:loading(editor.get_game_world()) and not loading(editor.get_game_world()).is_loading()
    check('transition_completed',str(unreal.GameplayStatics.get_player_controller(editor.get_game_world(),0).get_hud().screen.get_page())=='hud')
    yield delay(.5)
    capture_scene('after-loading')
    levels.editor_request_end_play()
    yield lambda:not levels.is_in_play_in_editor()
    report['passed']=True
iterator=run();pending=None;deadline=time.monotonic()+240
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Loading UI verification')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:
        (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
