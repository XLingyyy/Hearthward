"""TASK-058: real construction and Camp UI; isolated flat-world resources, no normal keyboard or natural-map acceptance."""
import json, time, traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task058/pie';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Actual PIE components and UI actions; fixture resources; no natural-map or physical-input claim'}
st={}
frames=[]
(out/'frames').mkdir(exist_ok=True)
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=20):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+5)
def guid(value):
    result=unreal.GuidLibrary.parse_string_to_guid(value)
    return result[0] if isinstance(result,tuple) else result
def state():return json.loads(st['camp'].describe())
def facility(kind):return next(f for f in state()['facilities'] if f['kind']==kind)
def region(fid):return next(r for r in state()['regions'] if r['facility']==fid)
def epoch():return st['store'].get_timeline_epoch()
def shot(name):
    unreal.SystemLibrary.execute_console_command(st['world'],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st['pc'])
def grant(item,count):
    while count:
        n=min(8,count)
        assert st['bag'].try_add(item,n)==unreal.HearthwardInventoryResult.SUCCESS,(item,n)
        r=st['access'].transfer(st['bag'],True,item,n,unreal.GuidLibrary.new_guid(),epoch())
        assert r.moved_count==n,(item,r)
        count-=n
def locate():
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),
        bag=p.get_component_by_class(unreal.HearthwardInventoryComponent),
        game=p.get_component_by_class(unreal.HearthwardGameplayComponent),
        builder=p.get_component_by_class(unreal.HearthwardBuildingComponent))
    for key,cls in [('camp',unreal.HearthwardCampSubsystem),('store',unreal.HearthwardStorageSubsystem),('save',unreal.HearthwardSaveSubsystem),('clock',unreal.HearthwardWorldClockSubsystem)]:
        st[key]=next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def safe_targets():
    for i,a in enumerate(unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.Actor)):
        t=a.get_component_by_class(unreal.HearthwardCombatTargetComponent)
        if t:a.set_actor_location(unreal.Vector(50000+i*1000,0,100),False,True)
def go(x,y):
    st['ui'].open_page('hud');st['pawn'].set_actor_location(unreal.Vector(x,y,100),False,True)
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
def build(kind,x,y):
    go(x,y);yield delay(.6)
    before=st['builder'].building_count()
    check('select '+kind,st['builder'].select_building(kind));yield delay(.3)
    report['placement_'+kind]=str(st['builder'].feedback)
    check('start '+kind,st['builder'].confirm_placement())
    yield wait(lambda:st['builder'].building_count()==before+1,9)
    check('five second construction '+kind,not st['builder'].is_building())
def finish(error=None):
    if error:report['error']=error
    report['ok']=not error and all(report['checks'].values())
    try:
        if st.get('camp'):report['final_state']=state()
        if st.get('save'):report['save_status']=st['save'].get_status()
        if st.get('clock'):report['clock_calendar']=st['clock'].get_snapshot().elapsed_calendar_minutes
    except Exception:
        report['observer_error']=traceback.format_exc();report['ok']=False
    (out/'frame-times.json').write_text(json.dumps(frames),encoding='utf-8')
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(9000,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(400,200,1));floor.static_mesh_component.set_collision_profile_name('BlockAll')
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    locate();unreal.GameplayStatics.set_game_paused(st['world'],False)
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Companion.CreateTest',st['pc']);yield delay(.5)
    brother=unreal.GameplayStatics.get_actor_of_class(st['world'],unreal.HearthwardCompanionFixture)
    brother.set_actor_location(unreal.Vector(-650,-650,100),False,True)
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Storage.CreateTestAccess',st['pc'])
    st['access']=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world()==st['world'])
    st['game'].enable_adventure();safe_targets();st['game'].order_companion('wait')
    brother.set_actor_location(st['pawn'].get_actor_location()+unreal.Vector(150,0,0),False,True)
    yield wait(lambda:brother.get_component_by_class(unreal.CharacterMovementComponent).get_editor_property('movement_mode')==unreal.MovementMode.MOVE_WALKING,8)
    report['initial_brother']={'location':str(brother.get_actor_location()),'life':str(brother.get_component_by_class(unreal.HearthwardSurvivalComponent).state.life)}
    check('enable isolated saves',st['save'].enable_prototype())
    check('new progress',st['save'].start_new_progress())
    check('initial twenty people and one camp',len(state()['rescued'])==0 and len(state()['camps'])==1 and state()['tier']==1)
    for item,count in [('wood',564),('stone',264),('rope',20),('wild_food',16)]:grant(item,count)
    # Start, prevent theft, then interrupt before any construction output.
    go(-200,-200);yield delay(.6);st['builder'].select_building('workbench');yield delay(.3)
    before=st['store'].get_item_count('wood')
    check('reserve build',st['builder'].confirm_placement())
    check('no upfront visible debit',st['store'].get_item_count('wood')==before)
    check('save blocks construction reservation',not st['save'].save_point(True))
    yield delay(.3);st['pawn'].set_actor_location(st['pawn'].get_actor_location()+unreal.Vector(30,0,0),False,True);yield delay(.3)
    check('movement interruption releases without cost',not st['builder'].is_building() and st['store'].get_item_count('wood')==before)
    yield from build('workbench',-200,-200)
    check('workbench exact 72 wood',st['store'].get_item_count('wood')==before-72)
    bench=facility('workbench');bid=guid(bench['id']);rid=region(bench['id'])['id']
    check('rescue fact accepted once',st['camp'].record_rescue('fixture_rescue') and not st['camp'].record_rescue('fixture_rescue'))
    st['ui'].open_page('camp');check('growth tab action',st['ui'].execute_action('camp.tab:growth'))
    check('tier 2 through real UI',st['ui'].execute_action('camp.upgrade') and state()['tier']==2)
    shot('growth');yield delay(.5)
    yield from build('cooking',-200,200)
    check('donate real shared food',st['camp'].donate_food('wild_food',16,epoch()))
    check('tier 3 conditions and cost',st['camp'].upgrade_camp(epoch()) and state()['tier']==3)
    go(-200,-200);yield delay(.6)
    check('configure processing',st['camp'].select_production(rid,bid,'rope',epoch()))
    check('assign processing person',st['camp'].assign_worker(rid,4,epoch()))
    check('enable processing',st['camp'].set_production(rid,True,False,epoch()))
    yield delay(.4)
    check('in-flight real batch',region(bench['id'])['batch']['active'])
    progress=region(bench['id'])['batch']['work']
    check('upgrade begins',st['builder'].upgrade_facility(bid,epoch()))
    yield delay(1)
    check('upgrade suspends batch',abs(region(bench['id'])['batch']['work']-progress)<.05)
    yield wait(lambda:not st['builder'].is_building(),8)
    check('facility II retains batch',facility('workbench')['level']==2 and region(bench['id'])['batch']['active'])
    st['ui'].open_page('camp');st['ui'].execute_action('camp.tab:facilities');yield delay(.3)
    layout=json.loads(st['ui'].describe_layout());text='\n'.join(c.get('text','') for c in layout['components'])
    check('actual facility II speed shown', '劳动速度 125%' in text)
    check('facility post shows real one body and batch', '工作区 1 / 5人' in text and '本批劳动' in text and '已投入' in text)
    shot('facilities');yield delay(.5)
    st['ui'].execute_action('camp.tab:growth');yield delay(.3)
    layout=json.loads(st['ui'].describe_layout());text='\n'.join(c.get('text','') for c in layout['components'])
    check('all eight camp tiers shown',all('S'+str(i)+'  ' in text for i in range(1,9)))
    check('actual next-tier radius shown', '下一阶半径 80 米' in text)
    shot('growth');yield delay(.5)
iterator=run();pending=None;deadline=time.monotonic()+300
def tick(dt):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('overall')
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('stage '+str(list(report['checks'])[-3:]))
            if not pending[0]():return
        pending=next(iterator)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
