"""Production-map integration. Explicit supplies/position/clearance fixtures, not a full playthrough."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task049/epilogue';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':'Natural map after permanent victory; real UI, harvesting, construction and camp transactions. Test supplies and scene/escort-edge relocation are explicit. No invented quest facts.'};st={}
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
def supply(item,count):check('fixture supply '+item,st['game'].grant_item_reward('fixture049epilogue:'+item,item,count,epoch()))
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
    check('continue victory checkpoint',st['ui'].execute_action('continue'));st['ui'].open_page('hud');yield delay(5)
    check('permanent victory survives editor restart',state()['victory']);st['freeze_enemies']=True
    # The last manual checkpoint precedes the reward claim; replay title continue and settle it once.
    if 'main_08' not in [str(x) for x in st['game'].claimed]:check('main08 after actual reopen',st['game'].claim('main_08'))
    yield from go('hometown');center=vec(state()['positions']['hometown']);radius=6000
    for index in range(2,11):
        identity='rescued_%02d'%index
        yield wait(lambda identity=identity:any(str(a.identity)==identity for a in actors()),15)
        person=actor(identity);near(person.get_actor_location());yield delay(1)
        check('contact '+identity,st['campaign'].interact())
        if identity not in camp()['rescued']:
            # Position fixture reduces escort duration; arrival itself uses AI walking and safe CampAt.
            person.set_actor_location(center+unreal.Vector(radius+250,800,0),False,True)
            p.set_actor_location(person.get_actor_location()+unreal.Vector(-180,0,0),False,True);yield delay(1)
            if next(x for x in state()['people'] if x['id']==identity)['stage']=='following':st['campaign'].interact()
            check('resume '+identity,st['campaign'].interact())
            p.set_actor_location(center+unreal.Vector(radius-900,800,80),False,True)
            yield wait(lambda identity=identity:identity in camp()['rescued'],45)
        check('unique arrival '+identity,len(camp()['rescued'])==index)
    check('thirty ordinary citizens',len(camp()['rescued'])+20==30)
    facts=[str(x) for x in st['game'].reward_facts];check('rescue five unique receipt','reward:rescue5' in facts)
    check('exactly one milestone bow in shared storage',st['store'].get_item_count('bow_rare')==1)
    xp=st['game'].experience;check('duplicate rescue rejected',not st['camp'].record_rescue('rescued_05'));check('duplicate rescue changes no XP',st['game'].experience==xp)
    for q in ['side_01','side_02','side_03','side_04']:
        check('post victory rescue quest '+q,st['game'].claim(q));check('rescue reward repeat rejected '+q,not st['game'].claim(q))
    yield from go('route_ridge');check('ridge interaction',st['campaign'].interact());check('ridge quest',st['game'].claim('side_08'))
    yield from go('camp');yield delay(2)
    points=nature()['points'];herb=next(x for x in points if x['definition']=='herb_patch');yield from harvest(herb)
    yield from go('camp');before=st['store'].get_item_count('herb');check('explicit herb delivery',st['game'].claim('side_09'));check('herb deduction exactly six',st['store'].get_item_count('herb')==before-6);check('herb delivery repeat rejected',not st['game'].claim('side_09'))
    yield from go('loot_workshops');check('workshop cache interaction',st['campaign'].interact())
    yield from go('camp',unreal.Vector(-1600,-1800,120))
    for item,count in [('wood',170),('stone',150),('metal_ingot',20),('cooked_food',40)]:supply(item,count)
    builder=p.get_component_by_class(unreal.HearthwardBuildingComponent);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
    check('select forge',builder.select_building('forge'));yield delay(.8);check('build forge',builder.confirm_placement());yield wait(lambda:not builder.is_building(),12)
    facility=next(x for x in camp()['facilities'] if x['kind']=='forge');report['forge']=facility
    near(builder.get_buildings()[-1].get_actor_location());yield delay(1);st['ui'].open_page('camp');check('open real forge catalog',st['ui'].execute_action('camp.facility:'+facility['id']));st['ui'].open_page('hud');check('forge quest',st['game'].claim('side_12'))
    check('donate actual food',st['camp'].donate_food('cooked_food',40,epoch()))
    yield from go('camp',unreal.Vector(-2000,1800,120));pc.set_control_rotation(unreal.Rotator(yaw=0));yield delay(1)
    brother=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0];brother.set_actor_location(p.get_actor_location()+unreal.Vector(0,250,0),False,True);st['game'].order_companion('wait')
    st['bag'].try_add('seed_grain',1)
    check('plant real grain',st['nature'].act('plant',unreal.Guid(),'grain',epoch(),1));yield wait(lambda:not st['nature'].busy(),12)
    crop=next(x for x in nature()['crops'] if x['definition']=='grain');report['crop']=crop
    for i in range(9):
        check('calendar advances '+str(i),st['clock'].advance_calendar(480)>=479.99);yield delay(.1)
        if i%2==1:
            check('actual player meal '+str(i),st['camp'].eat_meal(False,epoch()));check('actual brother meal '+str(i),st['camp'].eat_meal(True,epoch()))
    near(vec(crop['position']));yield delay(1)
    check('harvest mature grain',st['nature'].act('harvest',guid(crop['id']),'',epoch(),1));yield wait(lambda:not st['nature'].busy(),12)
    check('grain harvest settled',str(st['nature'].feedback)=='操作完成');check('grain quest',st['game'].claim('side_11'));check('public food quest',st['game'].claim('side_15'))
    check('all eight mains and fifteen sides claimed',all(q['id'] in [str(x) for x in st['game'].claimed] for q in data['campaign']['quests']))
    yield from go('camp');check('save complete campaign',st['save'].save_point(True));st['ui'].open_page('journal');shot('all-quests');yield delay(.5);finish()
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
