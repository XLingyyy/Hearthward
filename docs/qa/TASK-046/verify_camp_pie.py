"""TASK-046: isolated PIE, explicit material/source/victory fixtures; no asset edits."""
import json, time, traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task046/pie';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Actual PIE components and UI actions; fixture resources and victory; no natural-map or physical-input claim'}
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
    if st.get('camp'):report['final_state']=state()
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
    st['game'].enable_adventure();safe_targets();yield delay(.8)
    check('enable isolated saves',st['save'].enable_prototype())
    check('new progress',st['save'].start_new_progress())
    check('initial twenty people and one camp',len(state()['rescued'])==0 and len(state()['camps'])==1 and state()['tier']==1)
    for item,count in [('wood',900),('stone',450),('ore',200),('rope',30),('wild_food',40)]:grant(item,count)
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
    yield from build('bed',-650,200)
    check('donate real shared food',st['camp'].donate_food('wild_food',16,epoch()))
    check('tier 3 conditions and cost',st['camp'].upgrade_camp(epoch()) and state()['tier']==3)
    go(-200,-200);yield delay(.6)
    rope=st['store'].get_item_count('rope');wood=st['store'].get_item_count('wood')
    check('manual craft instantaneous',st['camp'].craft(bid,'rope',1,epoch()))
    check('manual exact exchange',st['store'].get_item_count('wood')==wood-2 and st['store'].get_item_count('rope')==rope+1)
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
    # Three explicit finite source patches; content binding remains TASK-048.
    site=state()['camps'][0]['position'];pos=unreal.Vector(site['x'],site['y'],site['z'])
    for i in range(3):check('source '+str(i),st['camp'].register_source('fixture_food_'+str(i),'wild_food',16,16,pos+unreal.Vector(1200,i*200,0),2880))
    for i in range(4):check('forage worker '+str(i),st['camp'].assign_worker('camp_forage',i,epoch()))
    check('explicit food destination',st['camp'].set_production('camp_forage',True,True,epoch()))
    st['ui'].open_page('camp');st['ui'].execute_action('camp.tab:workers');yield delay(.4);shot('workers');yield delay(.5)
    st['ui'].execute_action('camp.tab:facilities');yield delay(.4);shot('facilities');yield delay(.5)
    go(-650,200);yield delay(.6)
    clock=st['clock'].get_snapshot();hunger=st['game'].hunger
    bed=guid(facility('bed')['id']);before=state();before_rope=st['store'].get_item_count('rope')
    check('sleep actual eight hours',st['camp'].sleep(bed,epoch()))
    after=st['clock'].get_snapshot()
    check('sleep W plus 480 and A unchanged',abs(after.elapsed_calendar_minutes-clock.elapsed_calendar_minutes-480)<.001 and abs(after.active_play_seconds-clock.active_play_seconds)<.001)
    check('sleep ordinary forage completes five',state()['regions'][0]['completed']-before['regions'][0]['completed']==5)
    check('sleep processing completes at II speed',st['store'].get_item_count('rope')==before_rope+1)
    check('sleep hunger advances without automatic meal',abs(hunger-st['game'].hunger-480*100/2880)<.02)
    h=st['game'].hunger;r=state()['rationHalfPoints']
    check('public meal real consumption',st['camp'].eat_meal(False,epoch()) and state()['rationHalfPoints']==r-10 and st['game'].hunger>h)
    st['ui'].open_page('camp');st['ui'].execute_action('camp.tab:food');yield delay(.4);shot('food');yield delay(.5)
    st['ui'].open_page('hud')
    go(-200,-200);yield delay(.6)
    batch_before=region(bench['id'])['batch'];paid_before=facility('workbench')['paid']
    check('move actual facility starts',st['builder'].move_facility(bid,epoch()))
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-20,yaw=180));yield delay(.4)
    check('move valid placement',st['builder'].confirm_placement())
    yield wait(lambda:not st['builder'].is_building(),8)
    check('move preserves ledger and active input',facility('workbench')['paid']==paid_before and region(bench['id'])['batch']['inputs']==batch_before['inputs'])
    wood=st['store'].get_item_count('wood')
    check('active batch cancellation requires consent',not st['camp'].cancel_batch(rid,False,epoch()))
    check('confirmed cancellation loses input only',st['camp'].cancel_batch(rid,True,epoch()) and st['store'].get_item_count('wood')==wood)
    check('resume after cancellation',st['camp'].set_production(rid,True,False,epoch()))
    go(-200,200);yield delay(.6)
    cook=facility('cooking');wood=st['store'].get_item_count('wood');stone=st['store'].get_item_count('stone')
    check('actual demolition',st['builder'].demolish_facility(guid(cook['id']),True,epoch()))
    check('paid demolition floors per material',st['store'].get_item_count('wood')==wood+192 and st['store'].get_item_count('stone')==stone+96)
    check('save live economy',st['save'].save_point(True));point=st['save'].get_points()[-1].save_id
    saved=state();old=epoch()
    check('mutate then reload',st['camp'].donate_food('wild_food',1,epoch()) and st['save'].load_point(point))
    check('stale epoch rejected',not st['camp'].assign_worker('camp_forage',5,old))
    loaded=state()
    check('paid ledger and sources restored',loaded['facilities']==saved['facilities'] and loaded['sources']==saved['sources'])
    check('batch and rations restore exactly',loaded['regions']==saved['regions'] and loaded['rationHalfPoints']==saved['rationHalfPoints'])
    # Reclaim callback exercises shared state and zero-paid gift initialization.
    safe_targets();check('actual victory callback fixture',st['camp'].reclaim_hometown('fixture_victory',unreal.Vector(10000,0,0)))
    check('hometown has four zero-paid gifts',len([f for f in state()['facilities'] if f['camp']=='hometown' and not f['paid']])==4)
    check('one shared tier and rescued population',state()['tier']==3 and len(state()['rescued'])==1)
    check('save two camps',st['save'].save_point(True))
    saved=state();report['saved_state']=saved
    shot('world');yield delay(.6)
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.6)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    locate();check('new PIE continue from disk',st['ui'].execute_action('continue'));yield delay(.2)
    check('two camps and facilities survive disk',state()['facilities']==saved['facilities'] and len(state()['camps'])==2)
    check('rescue and shared tier survive disk',state()['tier']==3 and state()['rescued']==saved['rescued'])
    yield delay(.3)
iterator=run();pending=None;deadline=time.monotonic()+300
def tick(dt):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('overall')
        if st.get('world') and levels.is_in_play_in_editor() and (not frames or time.monotonic()-frames[-1]>.5):
            shot('frames/f%05d'%len(frames));frames.append(time.monotonic())
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('stage '+str(list(report['checks'])[-3:]))
            if not pending[0]():return
        pending=next(iterator)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
