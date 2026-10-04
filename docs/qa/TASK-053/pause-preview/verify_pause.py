"""Pause prototype verification in a disposable real PIE new game."""
import json, os, time, traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id=os.environ['HEARTHWARD_HUD_RUN']
out=Path(unreal.Paths.project_saved_dir())/'HUDPreview'/run_id
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report=dict(passed=False,checks={},captures=[],layouts=[])
def check(name,value):
    report['checks'][name]=bool(value)
    if not value: raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end
def ready():
    world=editor.get_game_world()
    if not world or unreal.GameplayStatics.get_current_level_name(world,True)!='L_HearthwardWilds':return False
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    if not pc or not pc.get_hud() or not pc.get_hud().screen:return False
    loading=next((s for s in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem) if s.get_outer()==unreal.GameplayStatics.get_game_instance(world)),None)
    return loading and not loading.is_loading() and str(pc.get_hud().screen.get_page())=='hud'
def capture(world,pc,name):
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.UI.CaptureHUDScene '+name,pc)
    report['captures'].append(name+'.png')
    return lambda:(out/(name+'.png')).is_file()
def layout(ui):
    rows=json.loads(ui.describe_layout())['components']
    report['layouts'].append(rows)
    return rows
def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world=editor.get_game_world()
    ui=unreal.GameplayStatics.get_player_controller(world,0).get_hud().screen
    check('disposable_new_game',ui.execute_action('new'))
    yield ready
    world=editor.get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    ui=pc.get_hud().screen
    yield delay(5)
    yield capture(world,pc,'world-before-pause')
    check('pause_entry',ui.execute_action('page:pause') and str(ui.get_page())=='pause')
    check('world_frozen',unreal.GameplayStatics.is_game_paused(world))
    rows=layout(ui)
    options=[r for r in rows if r['id'].startswith('pause.option.') and r['visible']]
    check('original_five_actions',[r['action'] for r in options]==['page:hud','continuePrompt','save','page:settings','ask:title'])
    check('vertical_order',len({r['rect'][0] for r in options})==1 and [r['rect'][1] for r in options]==sorted(r['rect'][1] for r in options))
    check('no_background_or_illustrations',not any(r.get('asset') in {'pauseBackground','leatherPanel','logo','pauseMountains','pauseTopOrnament','pauseBottomOrnament'} for r in rows))
    check('no_quest_or_logo',not any(r.get('component')=='pause.info' or '归火' in r.get('text','') or r.get('text')=='当前任务' for r in rows))
    check('settings_surface',any(r['id']=='pause.sheet' and r['rect']==[546,154,580,632] for r in rows))
    for i,r in enumerate(options):
        x,y,w,h=r['rect']
        check('hit_option_'+str(i),ui.action_at(unreal.Vector2D(x+w/2,y+h/2))==r['action'])
    yield delay(.5)
    yield capture(world,pc,'pause-world')
    check('settings_entry',ui.execute_action('page:settings') and str(ui.get_page())=='settings')
    check('settings_return',ui.execute_action('back') and str(ui.get_page())=='pause')
    check('still_paused_after_settings',unreal.GameplayStatics.is_game_paused(world))
    check('title_confirmation',ui.execute_action('ask:title'))
    check('confirmation_can_cancel',ui.execute_action('cancel') and str(ui.get_page())=='pause')
    check('resume_action',ui.execute_action('page:hud') and not unreal.GameplayStatics.is_game_paused(world))
    # A second camera angle demonstrates that blur samples the scene rather than an artwork.
    original=pc.get_control_rotation()
    pc.set_control_rotation(unreal.Rotator(original.pitch,original.yaw+55,original.roll))
    yield delay(.8)
    yield capture(world,pc,'world-second-angle')
    check('pause_second_angle',ui.execute_action('page:pause'))
    yield delay(.5)
    yield capture(world,pc,'pause-second-angle')
    check('return_action',ui.execute_action('back') and str(ui.get_page())=='hud' and not unreal.GameplayStatics.is_game_paused(world))
    # UI-only exports also verify scaling; blur must be assessed from viewport captures above.
    ui.open_page('pause')
    for name,w,h in [('pause-720p',1280,720),('pause-16x10',1600,1000),('pause-ultrawide',2560,1080)]:
        check('export_'+name,ui.capture_ui(run_id+'-'+name,w,h))
        report.setdefault('ui_captures',[]).append(run_id+'-'+name+'.png')
    levels.editor_request_end_play()
    yield lambda:not levels.is_in_play_in_editor()
    report['passed']=True
iterator=run()
pending=None
deadline=time.monotonic()+300
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Pause scene verification')
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
