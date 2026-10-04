"""Render and verify the split building UI in an isolated PIE fixture."""
import json,os,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
run_id=os.environ['HEARTHWARD_HUD_RUN']
out=Path(unreal.Paths.project_saved_dir())/'HUDPreview'/run_id
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report=dict(passed=False,checks={},captures=[],ui_captures=[],layouts=[])
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end
def rows(ui):return {r['id']:r for r in json.loads(ui.describe_layout())['components']}
def capture(ui,name,w=1672,h=941):
    filename=run_id+'-'+name
    check('capture_'+name,ui.capture_ui(filename,w,h))
    report['ui_captures'].append(filename+'.png')
def run():
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(2)
    world=editor.get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    ui=pc.get_hud().screen
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc)
    gameplay=unreal.GameplayStatics.get_player_pawn(world,0).get_component_by_class(unreal.HearthwardGameplayComponent)
    gameplay.enable_adventure()
    save=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    check('isolated_save_pool',save.enable_prototype())
    check('isolated_progress',save.start_new_progress())
    ui.open_page('hud')
    check('open_building',ui.execute_action('page:building'))
    r=rows(ui)
    check('left_surface',r['building.list.surface']['rect']==[40,136,664,706])
    check('right_surface',r['building.detail.surface']['rect']==[752,136,880,706])
    check('no_old_art',not any(x.get('asset') in {'pauseBackground','leatherPanel'} for x in r.values()))
    check('first_page_four',[x['action'] for x in r.values() if x['id'].startswith('building.list.row.')]==['building.select:workbench','building.select:campfire','building.select:bed','building.select:smelter'])
    check('first_prev_disabled',ui.action_at(unreal.Vector2D(120,812))=='')
    data=json.loads((Path(unreal.Paths.project_dir())/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
    for index,b in enumerate(data['buildings']):
        if index==4:check('page_next',ui.execute_action('buildNext'))
        check('select_'+b['id'],ui.execute_action('building.select:'+b['id']))
        r=rows(ui)
        check('original_name_'+b['id'],r['building.detail.name']['text']==b['name'])
        check('original_description_'+b['id'],r['building.detail.description']['text']==b['description'])
        check('original_build_action_'+b['id'],r['building.submit']['action']=='build:'+b['id'])
        check('one_selected_'+b['id'],sum(x.get('text','').startswith('◆') for x in r.values() if x['id'].startswith('building.list.name.'))==1)
        report['layouts'].append(r)
        capture(ui,b['id'])
    check('last_next_disabled',ui.action_at(unreal.Vector2D(590,812))=='')
    check('previous',ui.execute_action('buildPrev'))
    ui.execute_action('building.select:workbench')
    for name,w,h in [('720p',1280,720),('16x10',1600,1000),('ultrawide',2560,1080)]:capture(ui,name,w,h)
    ui.execute_action('back')
    check('back',str(ui.get_page())=='hud')
    levels.editor_request_end_play()
    yield lambda:not levels.is_in_play_in_editor()
    report['passed']=True
iterator=run();pending=None;deadline=time.monotonic()+180
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Building UI verification')
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
