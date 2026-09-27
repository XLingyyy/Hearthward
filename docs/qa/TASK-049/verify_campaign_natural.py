"""Production natural map: actual new-game entry and physical prologue traversal."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task049/natural';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Production map, normal new-game UI, native interactions, player movement input; no terrain edits'};st={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=60):
    report['waiting_at']=traceback.extract_stack(limit=2)[0].lineno
    (out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+8)
def state():return json.loads(st['campaign'].describe())
def shot(name):unreal.SystemLibrary.execute_console_command(st['world'],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st['pc'])
def move(x,y):
    p=st['pawn'].get_actor_location();d=unreal.Vector(x-p.x,y-p.y,0)
    report['move_target']=[x,y];report['live_position']=[p.x,p.y,p.z]
    if 'house_collision' not in report:
        report['house_collision']=[{'actor':str(a.get_actor_location()),'name':c.get_name(),'at':str(c.get_world_location()),'extent':str(c.get_scaled_box_extent())} for a in unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.HearthwardTask028CampHouse) if 'CampaignPrologueHouse' in [str(t) for t in a.tags] for c in a.get_components_by_class(unreal.BoxComponent)]
    if time.monotonic()-st.get('heartbeat',0)>3:
        st['heartbeat']=time.monotonic();report['ahead_hit']=str(unreal.SystemLibrary.line_trace_single(st['world'],p+unreal.Vector(0,0,70),p+unreal.Vector(120,0,70),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[st['pawn']],unreal.DrawDebugTrace.NONE,True));(out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    if d.length()<7:return True
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-8,yaw=unreal.MathLibrary.deg_atan2(d.y,d.x)))
    st['pawn'].add_movement_input(d/d.length(),1,False);return False
def finish(error=None):
    if error:report['error']=error
    try:
        report['state']=state();report['position']=str(st['pawn'].get_actor_location());report['feedback']=str(st['campaign'].feedback);report['save_status']=st['save'].get_status()
    except Exception:pass
    report['ok']=not error and all(report['checks'].values())
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    editor_world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    editor_world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(8)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),game=p.get_component_by_class(unreal.HearthwardGameplayComponent),bag=p.get_component_by_class(unreal.HearthwardInventoryComponent))
    for key,cls in [('campaign',unreal.HearthwardCampaignSubsystem),('camp',unreal.HearthwardCampSubsystem),('store',unreal.HearthwardStorageSubsystem),('save',unreal.HearthwardSaveSubsystem)]:st[key]=next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
    check('normal new game starts',st['ui'].execute_action('new'))
    yield wait(lambda:not st['campaign'].busy(),120);yield delay(3)
    check('prologue loaded on terrain',state()['phase']=='prologue' and not p.get_movement_component().is_falling())
    check('main quest cannot be claimed before actions',not st['game'].claim('main_01'))
    check('amulet starts absent',st['bag'].get_item_count('amulet')==0)
    check('relic interaction',st['campaign'].interact());check('relic exists once',st['bag'].get_item_count('amulet')==1)
    st['campaign'].interact();check('relic retry no duplicate',st['bag'].get_item_count('amulet')==1)
    check('explicit brother follow',st['game'].order_companion('follow'));shot('prologue');yield delay(.8)
    for x,y in [(92000,53364),(92400,53364),(94000,53500),(94000,57000),(94000,62000),(90000,62000)]:
        yield wait(lambda x=x,y=y:move(x,y),25 if x==92400 and y==53364 else 100)
        check('player remains alive along back alley',st['game'].health>0)
        shot('walk-'+str(x)+'-'+str(y));yield delay(.3)
    brother=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    yield wait(lambda:(brother.get_actor_location()-p.get_actor_location()).length()<900,40)
    check('exit interaction',st['campaign'].interact());yield wait(lambda:state()['phase']=='occupied' and not st['campaign'].busy(),120)
    camp_state=json.loads(st['camp'].describe())
    check('initial population remains twenty',camp_state['rescued']==[])
    site=camp_state['camps'][0]['position'];check('economic camp matches physical camp',abs(site['x']+98000)<1 and abs(site['y']+75000)<1)
    before=st['game'].experience;check('main quest reward',st['game'].claim('main_01'));check('main experience is 2000',st['game'].experience-before==2000)
    check('duplicate main reward rejected',not st['game'].claim('main_01'))
    yield delay(3);shot('camp-after-prologue');yield delay(.8)
    previous_ids={unreal.GuidLibrary.conv_guid_to_string(x.save_id) for x in st['save'].get_points()}
    check('safe checkpoint',st['save'].save_point(True));report['checkpoint']=unreal.GuidLibrary.conv_guid_to_string(next(x.save_id for x in st['save'].get_points() if unreal.GuidLibrary.conv_guid_to_string(x.save_id) not in previous_ids));finish()
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError('Natural campaign condition timed out')
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
