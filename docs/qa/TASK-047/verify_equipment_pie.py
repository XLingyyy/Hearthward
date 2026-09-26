"""TASK-047: isolated PIE, explicit material/source/victory fixtures; no asset edits."""
import json, time, traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task047/pie';out.mkdir(parents=True,exist_ok=True)
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
def items(bag=None):return json.loads((bag or st['bag']).describe_inventory())
def find_instance(identity,bag=None):return next(i for i in items(bag)['instances'] if i['id']==identity)
def select(identity):
    st['ui'].open_page('equipment');check('select instance '+identity,st['ui'].execute_action('gear.select:'+identity))

def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(9000,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(400,200,1));floor.static_mesh_component.set_collision_profile_name('BlockAll')
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    locate();unreal.GameplayStatics.set_game_paused(st['world'],False)
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Companion.CreateTest',st['pc']);yield delay(.5)
    brother=unreal.GameplayStatics.get_actor_of_class(st['world'],unreal.HearthwardCompanionFixture);brother_bag=brother.get_editor_property('bag')
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Storage.CreateTestAccess',st['pc'])
    st['access']=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world()==st['world'])
    st['game'].enable_adventure();st['game'].grant_initial_equipment();safe_targets();yield delay(.5)
    check('starter kits both brothers',st['bag'].get_item_count('axe')==1 and brother_bag.get_item_count('axe')==1 and brother_bag.get_item_count('leggings')==1)
    initial=items();st['game'].grant_initial_equipment();check('kit claim idempotent',items()==initial)
    check('enable isolated saves',st['save'].enable_prototype());check('new progress',st['save'].start_new_progress())
    for item,count in [('wood',900),('stone',450),('ore',100),('rope',40),('hide',30)]:grant(item,count)
    yield from build('workbench',-200,-200)
    bench=facility('workbench');bid=guid(bench['id'])
    go(-200,-200);yield delay(.6)
    check('craft second axe',st['builder'].craft(bid,'stone_axe',1,epoch()))
    axes=[i for i in items()['instances'] if i['definition']=='axe'];check('two independent axes',len(axes)==2 and axes[0]['id']!=axes[1]['id'])
    first,second=axes[0]['id'],axes[1]['id']
    st['bag'].wear_instance(guid(first),50);st['bag'].wear_instance(guid(second),10)
    select(first);check('repair quarter via actual UI',st['ui'].execute_action('gear.repair:25'))
    check('repair only selected copy',find_instance(first)['durability']==50 and find_instance(second)['durability']==70)
    shot('equipment-repair');yield delay(.6)
    brother.set_actor_location(st['pawn'].get_actor_location()+unreal.Vector(150,0,0),False,True)
    check('transfer chosen copy to brother',st['ui'].execute_action('gear.to:brother'))
    check('identity and durability after transfer',find_instance(first,brother_bag)['durability']==50)
    check('select brother bag',st['ui'].execute_action('gear.owner:brother'));check('select brother exact instance',st['ui'].execute_action('gear.select:'+first))
    check('explicit brother equip',st['ui'].execute_action('gear.equip'));check('brother uses selected GUID',items(brother_bag)['equipped']['weapon']==first)
    shot('brother-equipment');yield delay(.4)
    check('return selected equipment',st['ui'].execute_action('gear.to:player'))
    check('select own bag',st['ui'].execute_action('gear.owner:player'));select(first)
    before_xp=st['game'].get_editor_property('experience')
    check('defeat XP awarded once',st['game'].grant_experience('guard','fixture:defeat',epoch()) and not st['game'].grant_experience('guard','fixture:defeat',epoch()))
    check('exact guard XP',st['game'].get_editor_property('experience')==before_xp+100)
    st['game'].set_editor_property('health',37);st['game'].set_editor_property('stamina',23)
    check('learn approved node',st['game'].learn('strong'))
    st['game'].reset_skills();check('free reset without refill',st['game'].get_editor_property('health')==37 and st['game'].get_editor_property('stamina')==23)
    check('rescue once',st['camp'].record_rescue('fixture_rescue') and not st['camp'].record_rescue('fixture_rescue'))
    check('camp tier two',st['camp'].upgrade_camp(epoch()))
    service=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.Character) if a.actor_has_tag('Hearthward.Quartermaster'))
    st['ui'].open_page('hud');st['pawn'].set_actor_location(service.get_actor_location()+unreal.Vector(130,0,0),False,True)
    brother.set_actor_location(service.get_actor_location()+unreal.Vector(130,130,0),False,True);yield delay(.4)
    st['ui'].open_page('equipment');check('upgrade my backpack UI',st['ui'].execute_action('gear.upgrade'));check('my rank2',st['bag'].get_capacity()==150)
    st['ui'].execute_action('gear.owner:brother');check('pay separately for brother backpack',st['ui'].execute_action('gear.upgrade'));check('brother rank2',brother_bag.get_capacity()==150)
    shot('backpacks');yield delay(.4)
    st['ui'].execute_action('gear.owner:player');select(first);check('drop selected item',st['ui'].execute_action('gear.drop'))
    ground=unreal.GameplayStatics.get_actor_of_class(st['world'],unreal.HearthwardDroppedEquipment)
    check('ground item actually exists',ground is not None)
    check('save equipment and ground',st['save'].save_point(True));snapshot=items();brother_snapshot=items(brother_bag)
    old_epoch=epoch();points=st['save'].get_points();saved_id=points[-1].save_id
    check('load current node',st['save'].load_point(saved_id))
    check('reject stale UI mutation',not st['game'].transfer_inventory('player','brother','arrow',1,unreal.Guid(),old_epoch))
    check('inventory exact restore',items()==snapshot and items(brother_bag)==brother_snapshot)
    ground=unreal.GameplayStatics.get_actor_of_class(st['world'],unreal.HearthwardDroppedEquipment)
    check('ground restored',ground is not None)
    st['ui'].open_page('hud');st['pawn'].set_actor_location(ground.get_actor_location()+unreal.Vector(20,0,95),False,True);yield delay(.3)
    interaction=st['pawn'].get_component_by_class(unreal.HearthwardInteractionComponent)
    check('pick up ground equipment through interaction',interaction.interact_nearest());yield delay(.2)
    check('pickup preserves original GUID and durability',find_instance(first)['durability']==50)
    st['ui'].open_page('equipment');shot('restored');yield delay(.5)
    check('save picked up state',st['save'].save_point(True));snapshot=items();brother_snapshot=items(brother_bag)
    levels.editor_request_end_play();st.clear();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.6)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    locate();check('fresh PIE disk continue',st['ui'].execute_action('continue'));yield delay(.3)
    brother=unreal.GameplayStatics.get_actor_of_class(st['world'],unreal.HearthwardCompanionFixture)
    check('identities durability capacities survive fresh PIE',items()==snapshot and items(brother.get_editor_property('bag'))==brother_snapshot)
    check('reward fact survives reload',not st['game'].grant_experience('guard','fixture:defeat',epoch()))
    st['ui'].open_page('equipment');shot('disk-restored');yield delay(.4)
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Storage.CreateTestAccess',st['pc'])
    st['access']=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world()==st['world'])
    for i in range(2,6):check('rescue reward fixture '+str(i),st['camp'].record_rescue('fixture_rescue'+str(i)))
    check('fifth rescue gives one rare bow',st['store'].get_item_count('bow_rare')==1)
    check('repeat rescue cannot duplicate reward',not st['camp'].record_rescue('fixture_rescue5') and st['store'].get_item_count('bow_rare')==1)
    check('blueprint quest reward callback',st['game'].grant_item_reward('claim:side_hunter_blueprint','blueprint_hunter_bow',1,epoch()))
    check('blueprint reward deduplicated',not st['game'].grant_item_reward('claim:side_hunter_blueprint','blueprint_hunter_bow',1,epoch()))
    check('blueprint withdrawal',st['access'].transfer(st['bag'],False,'blueprint_hunter_bow',1,unreal.GuidLibrary.new_guid(),epoch()).moved_count==1)
    check('learn blueprint',st['game'].use_item('blueprint_hunter_bow'))
    check('hometown unique reward callback',st['camp'].reclaim_hometown('fixture_victory',unreal.Vector(10000,0,0)))
    check('unique reward exactly once',st['store'].get_item_count('hearth_blade')==1 and not st['camp'].reclaim_hometown('fixture_victory',unreal.Vector(10000,0,0)))
    check('unique reward withdrawal',st['access'].transfer(st['bag'],False,'hearth_blade',1,unreal.GuidLibrary.new_guid(),epoch()).moved_count==1)
    unique=next(i for i in items()['instances'] if i['definition']=='hearth_blade')['id'];select(unique)
    check('unique drop requires confirmation',not st['game'].drop_equipment(guid(unique),epoch(),False))
    check('unique drop confirmation UI',st['ui'].execute_action('ask:gear.dropConfirmed'));shot('unique-confirm');yield delay(.4)
    check('confirmed unique drop',st['ui'].execute_action('confirm'))
    check('save unique ground and learned knowledge',st['save'].save_point(True));point=st['save'].get_points()[-1].save_id
    check('reload unique ground and reward ledger',st['save'].load_point(point))
    check('unique ground survives load',unreal.GameplayStatics.get_actor_of_class(st['world'],unreal.HearthwardDroppedEquipment) is not None)
    check('loaded unique cannot be awarded again',not st['game'].grant_item_reward('reward:hometown','hearth_blade',1,epoch()))

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
