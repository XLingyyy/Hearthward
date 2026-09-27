"""Production-map integration. Explicit supplies/position/clearance fixtures, not a full playthrough."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task049/continue';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Natural map; real UI/component transactions. Test supplies, scene relocation and garrison health fixtures are explicit. No invented quest facts.'};st={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=40):
    report['waiting_at']=traceback.extract_stack(limit=2)[0].lineno
    (out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+10)
def state():return json.loads(st['campaign'].describe())
def camp():return json.loads(st['camp'].describe())
def nature():return json.loads(st['nature'].describe())
def epoch():return st['store'].get_timeline_epoch()
def vec(p):return unreal.Vector(p['x'],p['y'],p['z'])
def guid(s):
    v=unreal.GuidLibrary.parse_string_to_guid(s)
    return v[0] if isinstance(v,tuple) else v
def actors():return unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.HearthwardCampaignActor)
def actor(identity):return next(a for a in actors() if str(a.identity)==identity)
def shot(name):unreal.SystemLibrary.execute_console_command(st['world'],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st['pc'])
def supply(item,count):check('fixture supply '+item,st['game'].grant_item_reward('fixture049:'+item,item,count,epoch()))
def go(location,offset=None):
    """Relocation fixture loads real cells and uses the campaign's projected terrain position."""
    st['ui'].open_page('hud');movement=st['pawn'].get_movement_component();movement.disable_movement()
    row=next(x for x in data['campaign']['locations'] if x['id']==location)
    st['pawn'].set_actor_location(unreal.Vector(row['xy'][0]*100,row['xy'][1]*100,60000),False,True)
    yield wait(lambda:location in state()['positions'],80);yield delay(3)
    position=vec(state()['positions'][location])+(offset or unreal.Vector(0,0,40))
    st['pawn'].set_actor_location(position,False,True);movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
    yield delay(1)
def near(position):
    st['ui'].open_page('hud');st['pawn'].set_actor_location(position+unreal.Vector(-120,0,100),False,True)
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-12,yaw=0))
def harvest(point):
    near(vec(point['position']));yield delay(1)
    check('harvest '+point['definition'],st['nature'].act('harvest',guid(point['id']),'',epoch(),1))
    yield wait(lambda:not st['nature'].busy(),12)
    check('harvest settled '+point['definition'],str(st['nature'].feedback)=='操作完成')
def finish(error=None):
    if error:report['error']=error
    try:
        report['state']=state();report['camp']=camp();report['game_feedback']=str(st['game'].feedback);report['feedback']=str(st['campaign'].feedback);report['save_status']=st['save'].get_status();report['position']=str(st['pawn'].get_actor_location());report['building_feedback']=str(st['pawn'].get_component_by_class(unreal.HearthwardBuildingComponent).feedback)
    except Exception:pass
    report['ok']=not error and all(report['checks'].values())
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    ew=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();ew.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(8)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),game=p.get_component_by_class(unreal.HearthwardGameplayComponent),bag=p.get_component_by_class(unreal.HearthwardInventoryComponent))
    for key,cls in [('campaign',unreal.HearthwardCampaignSubsystem),('camp',unreal.HearthwardCampSubsystem),('nature',unreal.HearthwardNatureSubsystem),('store',unreal.HearthwardStorageSubsystem),('save',unreal.HearthwardSaveSubsystem),('clock',unreal.HearthwardWorldClockSubsystem)]:st[key]=next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
    check('title continue',st['ui'].execute_action('continue'));st['ui'].open_page('hud');yield delay(5)
    check('victory checkpoint present',state()['victory'])
    yield from go('hometown',unreal.Vector(200,0,40));yield delay(2)
    check('checkpoint before final UI checks',st['save'].save_point(True))
    check('home warehouse opens',st['ui'].execute_action('page:storage'));st['ui'].open_page('hud')
    check('home worker assignment',st['camp'].assign_worker('hometown_wood',1,epoch()))
    previous_ids={unreal.GuidLibrary.conv_guid_to_string(x.save_id) for x in st['save'].get_points()}
    check('save reclaimed world',st['save'].save_point(True));saved=next(x.save_id for x in st['save'].get_points() if unreal.GuidLibrary.conv_guid_to_string(x.save_id) not in previous_ids);old=epoch();before=state()
    check('load reclaimed world',st['save'].load_point(saved));yield delay(1)
    check('victory and flags survive',state()['victory'] and state()['flags']==before['flags'])
    check('arrived identity restored',next(x for x in state()['people'] if x['id']=='rescued_01')['stage']=='arrived')
    check('old timeline quest rejected',not st['campaign'].claim('side_15',old))
    check('ordinary reload does not change continue fact',('home_continued' in state()['facts'])==('home_continued' in before['facts']))
    st['ui'].open_page('title');check('title continue after victory',st['ui'].execute_action('continue'));st['ui'].open_page('hud');yield delay(1)
    check('title continue fact','home_continued' in state()['facts']);check('main08 closes eight main quests',st['game'].claim('main_08'));shot('reclaimed');yield delay(.5);finish()
data=json.loads((Path(unreal.Paths.project_dir())/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if st.get('freeze_enemies'):
            for a in actors():
                t=a.get_component_by_class(unreal.HearthwardCombatTargetComponent)
                if not t.protected:a.set_actor_tick_enabled(False);t.set_editor_property('exposure',0)
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError('World integration condition timed out')
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
