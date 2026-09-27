"""Production-map integration. Explicit supplies/position/clearance fixtures, not a full playthrough."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task049/world';out.mkdir(parents=True,exist_ok=True)
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
    check('prologue checkpoint resumed',state()['phase']=='occupied' and 'main_01' in [str(x) for x in st['game'].claimed])
    st['freeze_enemies']=True
    points=nature()['points']
    yield from harvest(next(x for x in points if x['definition']=='fallen_branches'))
    yield from harvest(next(x for x in points if x['definition']=='loose_stones'))
    yield from go('camp',unreal.Vector(1800,-1800,120))
    for item,count in [('wood',240),('stone',100),('rope',20),('ore',6),('herb',6),('cooked_food',5)]:supply(item,count)
    builder=p.get_component_by_class(unreal.HearthwardBuildingComponent);st['pc'].set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
    check('select workbench',builder.select_building('workbench'));yield delay(.8)
    report['placement_feedback']=str(builder.feedback);check('pay and build workbench',builder.confirm_placement());yield wait(lambda:not builder.is_building(),12)
    check('one workbench recorded',any(x['kind']=='workbench' for x in camp()['facilities']))
    region=next(x['id'] for x in camp()['regions'] if x['id'].endswith('wood'))
    check('real worker assignment',st['camp'].assign_worker(region,0,epoch()))
    brother=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    brother.set_actor_location(p.get_actor_location()+unreal.Vector(150,0,0),False,True)
    check('camp brother command',st['game'].order_companion('wait'))
    check('main02 real facts',st['game'].claim('main_02'))
    yield from go('slice_rescue');yield delay(2)
    check('rescue contact',st['campaign'].interact());yield delay(1)
    check('contact alone gives no population','rescued_01' not in camp()['rescued'])
    person=actor('rescued_01');report['rescue_source_position']=str(person.get_actor_location())
    # Arrival fixture shortens the 650m return while retaining actual AI walk and CampAt settlement.
    yield from go('camp',unreal.Vector(400,0,100));camp_position=vec(state()['positions']['camp'])
    radius=float(data['campEconomy']['camp_tiers'][0]['radius_m'])*100
    person=actor('rescued_01') if any(str(a.identity)=='rescued_01' for a in actors()) else None
    check('rescuer remains loaded within 650m',person is not None)
    person.set_actor_location(camp_position+unreal.Vector(radius+300,0,0),False,True)
    p.set_actor_location(person.get_actor_location()+unreal.Vector(-180,0,0),False,True);yield delay(1)
    if next(x for x in state()['people'] if x['id']=='rescued_01')['stage']=='following':st['campaign'].interact()
    check('resume rescue follow',st['campaign'].interact())
    p.set_actor_location(camp_position+unreal.Vector(radius-800,0,30),False,True)
    yield wait(lambda:'rescued_01' in camp()['rescued'],50)
    check('single arrival adds one citizen',len(camp()['rescued'])==1)
    yield from go('camp');check('main03 arrival facts',st['game'].claim('main_03'))
    check('tier two actual payment',st['camp'].upgrade_camp(epoch()));st['ui'].open_page('camp');st['ui'].open_page('hud')
    check('main04 upgrade facts',st['game'].claim('main_04'))
    yield from go('route_mine');check('real mine point',any(x['key']=='campaign_route_mine' for x in nature()['points']))
    yield from go('route_ford');check('ford activate',st['campaign'].interact());check('side07 reward',st['game'].claim('side_07'));check('side07 repeat rejected',not st['game'].claim('side_07'))
    yield from go('route_watch');check('watch survey',st['campaign'].interact());yield from go('home_entry')
    check('main05 actual discovery',st['game'].claim('main_05'))
    yield from go('hometown');yield delay(5)
    loaded=[a for a in actors() if str(a.identity).startswith('home_')]
    check('all eighty loaded from stable registry',len(loaded)==80)
    report['enemy_meshes']={str(a.identity):str(a.mesh.skeletal_mesh) for a in loaded[:2]};shot('occupied');yield delay(.5)
    report['actor_bounds']={}
    kinds={e['id']:e['kind'] for e in state()['enemies']}
    arm=p.get_component_by_class(unreal.SpringArmComponent);old_arm=arm.target_arm_length
    camera=p.get_component_by_class(unreal.CameraComponent);old_camera=camera.get_editor_property('relative_location')
    p.get_movement_component().disable_movement();p.set_actor_hidden_in_game(True);arm.target_arm_length=0
    camera.set_relative_location(unreal.Vector(0,0,40),False,True)
    for kind in ['guard','heavy']:
        subject=next(a for a in loaded if kinds[str(a.identity)]==kind);at=subject.get_actor_location();camera_at=at+unreal.Vector(400,0,40)
        p.set_actor_location(camera_at-unreal.Vector(0,0,40),False,True);pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(camera_at,at))
        yield delay(.5);shot(kind+'-runtime');yield delay(.5)
        report['actor_bounds'][kind]=str(subject.get_actor_bounds(False))
    arm.target_arm_length=old_arm;camera.set_relative_location(old_camera,False,True);p.set_actor_hidden_in_game(False);p.get_movement_component().set_movement_mode(unreal.MovementMode.MOVE_WALKING)

    target=loaded[0]
    # Clearance fixture isolates the actual execution from other guards' detection cones.
    for a in actors():
        if a!=target and str(a.identity).startswith(('home_','reinforce_')):a.get_component_by_class(unreal.HearthwardCombatTargetComponent).health=0
    target.get_controller().stop_movement();target.set_actor_rotation(unreal.Rotator(yaw=0),False)
    p.set_actor_location(target.get_actor_location()+unreal.Vector(-110,0,10),False,True);st['pc'].set_control_rotation(unreal.Rotator(yaw=0));yield delay(.8)
    shot('enemy-close');yield delay(.5)
    combat=p.get_component_by_class(unreal.HearthwardCombatComponent);yield wait(lambda:combat.discovery<.01,30);yield delay(2);xp=st['game'].experience
    check('actual human execution starts',combat.execute(target));yield wait(lambda:not combat.busy(),10)
    report['execution_feedback']=str(combat.feedback);report['execution_health']=target.get_component_by_class(unreal.HearthwardCombatTargetComponent).health
    check('actual human execution commits health and XP',target.get_component_by_class(unreal.HearthwardCombatTargetComponent).health==0 and st['game'].experience>xp)
    # Exhaustive K/N variants are native-tested; this PIE fixture tests flags and real world conversion.
    for a in actors():
        if str(a.identity).startswith(('home_','reinforce_')):a.get_component_by_class(unreal.HearthwardCombatTargetComponent).health=0
    yield delay(1);check('cleared ledger retained',sum(x['combat']['health']==0 for x in state()['enemies'] if x['group']=='base')==80)
    check('both reinforcement groups terminal','pending' not in state()['reinforcements'].values());report['reinforcement_outcome']=state()['reinforcements']
    yield wait(lambda:not st['game'].in_combat(),45)
    for index,zone in enumerate(['river_gate','workshops','dwellings','assembly']):
        yield from go('loc_'+zone)
        if index==0:
            check('start flag',st['campaign'].interact());check('flag blocks save',not st['save'].save_point(True));check('flag blocks calendar skip',st['clock'].advance_calendar(480)==0)
            p.set_actor_location(p.get_actor_location()+unreal.Vector(100,0,0),False,True);yield delay(1)
            check('movement interrupts flag',zone not in state()['flags'] and not st['campaign'].busy())
        check('flag interact '+zone,st['campaign'].interact());yield wait(lambda:not st['campaign'].busy(),12)
        check('flag completes '+zone,zone in state()['flags'])
    yield wait(lambda:state()['victory'],15)
    check('victory without claiming main06 or main07',not any(str(x) in ['main_06','main_07'] for x in st['game'].claimed))
    check('home gift facilities atomically created',sum(x['camp']=='hometown' for x in camp()['facilities'])==4)
    check('main06 reward',st['game'].claim('main_06'));check('main07 reward',st['game'].claim('main_07'))
    yield from go('loot_river_gate');st['campaign'].interact();yield from go('loot_dwellings');st['campaign'].interact();yield from go('loot_assembly');st['campaign'].interact()
    for location,quest in [('camp_memorial','side_10'),('civilian_initial_01','side_13'),('camp_records','side_14'),('camp_hunter','side_05')]:
        yield from go(location);check('safe interaction '+location,st['campaign'].interact());check('safe side quest '+quest,st['game'].claim(quest));check('safe side repeat '+quest,not st['game'].claim(quest))
    yield from go('camp');check('explicit ore delivery',st['game'].claim('side_06'));check('ore delivery repeat rejected',not st['game'].claim('side_06'))
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
    check('save reload does not pretend title continue','home_continued' not in state()['facts'])
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
