"""Actual PIE integration with explicitly granted test supplies and a flat fixture floor."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task048/pie';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Actual PIE/UI/component actions; explicit flat-floor and inventory fixtures; natural terrain tested separately'}
st={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=25):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+8)
def guid(s):
    value=unreal.GuidLibrary.parse_string_to_guid(s)
    return value[0] if isinstance(value,tuple) else value
def state():return json.loads(st['nature'].describe())
def camp():return json.loads(st['camp'].describe())
def items():return json.loads(st['bag'].describe_inventory())
def epoch():return st['store'].get_timeline_epoch()
def locate():
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),bag=p.get_component_by_class(unreal.HearthwardInventoryComponent),game=p.get_component_by_class(unreal.HearthwardGameplayComponent))
    for key,cls in [('nature',unreal.HearthwardNatureSubsystem),('camp',unreal.HearthwardCampSubsystem),('store',unreal.HearthwardStorageSubsystem),('save',unreal.HearthwardSaveSubsystem),('clock',unreal.HearthwardWorldClockSubsystem)]:
        st[key]=next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def pos(v):return unreal.Vector(v['x'],v['y'],v['z'])
def go(v):
    st['ui'].open_page('hud');st['pawn'].set_actor_location(v+unreal.Vector(-150,0,120),False,True)
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-15,yaw=0))
def add(item,n):check('grant '+item+str(n),st['bag'].try_add(item,n)==unreal.HearthwardInventoryResult.SUCCESS)
def grant_shared(item,n):
    while n:
        count=min(8,n);add(item,count)
        r=st['access'].transfer(st['bag'],True,item,count,unreal.GuidLibrary.new_guid(),epoch())
        check('deposit fixture '+item,r.moved_count==count);n-=count
def shot(name):unreal.SystemLibrary.execute_console_command(st['world'],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st['pc'])
def action(command,target,option=''):
    st['ui'].open_nature(guid(target) if target else unreal.Guid())
    check('start '+command,st['ui'].execute_action('nature.'+command+':'+option))
    yield wait(lambda:not st['nature'].busy(),10)
    check('settle '+command,str(st['nature'].feedback)=='操作完成')
def finish(error=None):
    if error:report['error']=error
    report['ok']=not error and all(report['checks'].values())
    try:
        report['state']=state();report['feedback']=str(st['nature'].feedback);report['save_status']=st['save'].get_status()
    except Exception:report['observer_error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(1600,1600,1));floor.static_mesh_component.set_collision_profile_name('BlockAll');floor.tags=['Hearthward.NatureGround']
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    locate();unreal.GameplayStatics.set_game_paused(st['world'],False)
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Companion.CreateTest',st['pc']);yield delay(.5)
    st['game'].enable_adventure();st['game'].grant_initial_equipment()
    # Human encounter fixtures are kept away from production tests.
    for a in unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.Actor):
        t=a.get_component_by_class(unreal.HearthwardCombatTargetComponent)
        if t and not isinstance(a,unreal.HearthwardNatureActor):a.set_actor_location(unreal.Vector(150000,150000,500),False,True)
    yield delay(1)
    check('enable saves',st['save'].enable_prototype());check('new progress',st['save'].start_new_progress());yield delay(.6)
    s=state();check('eight wildlife species',len(set(a['definition'] for a in s['animals'] if not a['domestic']))==8)
    check('twelve finite starter livestock',sum(a['domestic'] for a in s['animals'])==12)
    check('four finite fishing spots',sum(p['kind']=='fish' for p in s['points'])==4)
    branches=next(p for p in s['points'] if p['definition']=='fallen_branches')
    go(pos(branches['position']));yield delay(.6)
    before=st['bag'].get_item_count('wood');yield from action('harvest',branches['id'])
    check('bare hand restart wood',st['bag'].get_item_count('wood')==before+2)
    source=next(x for x in camp()['sources'] if x['id']==branches['key']);check('shared source debited',source['remaining']==14)
    go(unreal.Vector(-1800,-1500,0));yield delay(1);add('seed_greens',1)
    yield from action('plant','','greens');crop=state()['crops'][0]
    go(pos(crop['position']));yield delay(.6);yield from action('water',crop['id']);yield from action('fertilize',crop['id'])
    check('shared care flags',state()['crops'][0]['watered'] and state()['crops'][0]['fertilized'])
    add('wild_food',30)
    for i in range(6):
        check('calendar fixture advances W '+str(i),st['clock'].advance_calendar(480)>479)
        for j in range(3):st['game'].use_item('wild_food')
    before=st['bag'].get_item_count('wild_food');yield from action('harvest',crop['id'])
    check('care harvest six and returns seed',st['bag'].get_item_count('wild_food')==before+6 and st['bag'].get_item_count('seed_greens')==1 and not state()['crops'])
    spot=next(p for p in state()['points'] if p['kind']=='fish');go(pos(spot['position']));yield delay(.6)
    add('fishing_rod',1);add('bait',5)
    st['ui'].open_nature(guid(spot['id']));shot('fishing-panel');yield delay(.3)
    check('start fishing UI',st['ui'].execute_action('nature.fish:'))
    check('save blocked while fishing',not st['save'].save_point(True));check('sleep blocked while fishing',st['clock'].advance_calendar(480)==0)
    yield delay(1.2);st['nature'].cancel();check('bait consumed after cast',st['bag'].get_item_count('bait')==4)
    check('cancel keeps stock',next(p for p in state()['points'] if p['id']==spot['id'])['remaining']==24)
    st['ui'].open_nature(guid(spot['id']));check('restart fishing',st['ui'].execute_action('nature.fish:'))
    end=time.monotonic()+18
    while st['nature'].is_fishing() and time.monotonic()<end:
        st['nature'].hold_line(st['nature'].fishing_tension()<.48)
        yield delay(.06)
    check('fishing succeeds with hold release',str(st['nature'].feedback).startswith('钓鱼成功'))
    check('one fish one source debit',sum(st['bag'].get_item_count('fish_'+x) for x in ['carp','crucian_carp','catfish','eel'])==1 and next(p for p in state()['points'] if p['id']==spot['id'])['remaining']==23)
    rod=next(i for i in items()['instances'] if i['definition']=='fishing_rod');check('only success wears rod',rod['durability']==39)
    st['ui'].open_nature(guid(spot['id']));shot('fish-success');yield delay(.3)
    add('treasure_map_1',1);check('read map through item use',st['game'].use_item('treasure_map_1'))
    treasure=next(p for p in state()['points'] if p['kind']=='treasure')
    go(pos(treasure['position']));yield delay(1);bow_before=st['bag'].get_item_count('bow_2');yield from action('claim',treasure['id'])
    check('treasure yields approved bow and ingots',st['bag'].get_item_count('bow_2')==bow_before+1 and st['bag'].get_item_count('metal_ingot')>=8)
    check('treasure claim recorded once','treasure_map_1' in state()['opened'])
    deer=next(a for a in state()['animals'] if a['definition']=='deer' and a['health']>0)
    deer_actor=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.HearthwardNatureActor) if str(a.get_component_by_class(unreal.HearthwardCombatTargetComponent).get_editor_property('id'))==deer['id'])
    deer_actor.set_actor_tick_enabled(False);deer_actor.set_actor_location(unreal.Vector(-1500,1000,80),False,True);deer_actor.set_actor_rotation(unreal.Rotator(yaw=0),False)
    go(unreal.Vector(-1450,1000,0));yield delay(1)
    combat=st['pawn'].get_component_by_class(unreal.HearthwardCombatComponent)
    xp=st['game'].get_editor_property('experience');events=dict(st['game'].get_editor_property('events'))
    check('animal uses real three second execution',combat.execute(deer_actor));yield wait(lambda:not combat.busy(),8)
    check('wild death gives exactly thirty XP',st['game'].get_editor_property('experience')==xp+30)
    check('wild death never counts human clear',dict(st['game'].get_editor_property('events'))==events)
    yield delay(3.2);before=st['bag'].get_item_count('meat');yield from action('loot',deer['id'])
    check('deer corpse gives six meat',st['bag'].get_item_count('meat')==before+6)
    go(unreal.Vector(-200,-200,0));yield delay(1)
    unreal.SystemLibrary.execute_console_command(st['world'],'Hearthward.Storage.CreateTestAccess',st['pc'])
    st['access']=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageAccessComponent) if x.get_world()==st['world'])
    for item,count in [('wood',240),('stone',90),('rope',20)]:grant_shared(item,count)
    builder=st['pawn'].get_component_by_class(unreal.HearthwardBuildingComponent)
    check('select fixture workbench',builder.select_building('workbench'));yield delay(.4)
    check('build workbench prerequisite',builder.confirm_placement());yield wait(lambda:not builder.is_building(),10)
    check('rescue prerequisite',st['camp'].record_rescue('nature048_fixture_rescue'));check('camp tier two',st['camp'].upgrade_camp(epoch()))
    go(unreal.Vector(-2300,-2200,0));yield delay(1)
    yield from action('build_pen','','goat');pen=state()['pens'][0];check('pen exact paid ledger',pen['paid']=={'wood':60,'stone':20,'rope':10})
    add('feed',12);add('rope',2)
    animals=[a for a in state()['animals'] if a['domestic'] and a['definition']=='goat'][:2]
    for index,animal in enumerate(animals):
        actor=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(st['world'],unreal.HearthwardNatureActor) if str(a.get_component_by_class(unreal.HearthwardCombatTargetComponent).get_editor_property('id'))==animal['id'])
        # Move only the fixture animal to a reachable approach; actual capture and walking settle normally.
        actor.set_actor_tick_enabled(False);actor.set_actor_location(pos(pen['position'])+unreal.Vector(-650,index*180,100),False,True)
        go(actor.get_actor_location());yield delay(1)
        yield from action('capture',animal['id']);actor.set_actor_tick_enabled(True)
        check('reserved capture paid once '+str(index),next(a for a in state()['animals'] if a['id']==animal['id'])['captured'])
        go(pos(pen['position'])+unreal.Vector(-350,0,0));yield wait(lambda:next(a for a in state()['animals'] if a['id']==animal['id'])['pen']==pen['id'],15)
        check('animal walked into pen '+str(index),not next(a for a in state()['animals'] if a['id']==animal['id'])['following'])
    go(pos(pen['position']));yield delay(1);yield from action('deposit_feed',pen['id'])
    fed=state();check('feeder pays two whole goat windows',fed['pens'][0]['feed']==6 and all(a['fedRemaining']>1438 for a in fed['animals'] if a['pen']==pen['id']))
    add('wild_food',30)
    for i in range(6):
        check('livestock day advance '+str(i),st['clock'].advance_calendar(480)>479)
        for j in range(3):st['game'].use_item('wild_food')
    check('actual pen produces juvenile',any(a['juvenile'] and a['pen']==pen['id'] for a in state()['animals']))
    check('livestock never pauses safe camp production',all(r['safe'] for r in camp()['regions']))
    yield delay(.6)
    check('wild slot respawns new identity after two days',next(x for x in state()['slots'] if x['id']==deer['slot'])['current']!=deer['id'])
    st['ui'].open_nature(guid(pen['id']));shot('livestock-family');yield delay(.4)
    check('save schema7',st['save'].save_point(True));saved=st['save'].get_points()[-1].save_id;old=epoch();before=state()
    check('reload nature',st['save'].load_point(saved));check('exact nature restore',state()==before)
    check('old epoch rejected',not st['nature'].act('fish',guid(spot['id']),'',old,1))
    st['ui'].open_nature(guid(spot['id']));shot('nature-restored');yield delay(.4)
    disk=state();bag_disk=items()
    levels.editor_request_end_play();st.clear();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.6)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    locate();check('fresh PIE continue',st['ui'].execute_action('continue'));yield delay(.3)
    unreal.GameplayStatics.set_game_paused(st['world'],True)
    restored=state()
    check('fresh PIE keeps crops pens fish and reward claims',restored['points']==disk['points'] and restored['pens']==disk['pens'] and restored['rewards']==disk['rewards'])
    check('fresh PIE retains every animal identity',sorted(a['id'] for a in restored['animals'])==sorted(a['id'] for a in disk['animals']))
    check('fresh PIE keeps exact inventory instances',items()==bag_disk)
    st['ui'].open_nature(guid(pen['id']));shot('disk-restored-family');yield delay(.4)
    finish()
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError('PIE condition timed out')
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
