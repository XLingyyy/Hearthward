"""Isolated graybox PIE. Real UI actions, real local model, camp allocation and live production. Explicit resource and floor fixtures; no user save changes."""
from pathlib import Path
import json,time,traceback
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-081/pie'
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':__doc__,'model_cases':[]}
def flush(): (out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
def check(name,value):
    report['checks'][name]=bool(value);flush()
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end
def subsystem(cls,world):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
def goal(intent,item,quantity,mode,source):
    g=unreal.HearthwardAgentGoal()
    for k,v in dict(intent=intent,item=item,quantity=quantity,quantity_mode=mode,source_ref=source).items():g.set_editor_property(k,v)
    return g
def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(1600,1600,1));floor.static_mesh_component.set_collision_profile_name('BlockAll');floor.tags=['Hearthward.NatureGround']
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player=unreal.GameplayStatics.get_player_pawn(world,0);pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().get_editor_property('screen')
    unreal.GameplayStatics.set_game_paused(world,False)
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc);yield delay(.5)
    brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent);game.enable_adventure();game.order_companion('wait')
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent):actor.set_actor_location(unreal.Vector(150000,150000,500),False,True)
    save=subsystem(unreal.HearthwardSaveSubsystem,world);check('isolated prototype',save.enable_prototype());check('new isolated progress',save.start_new_progress())
    ai=subsystem(unreal.HearthwardLocalAISubsystem,world);camp=subsystem(unreal.HearthwardCampSubsystem,world);store=subsystem(unreal.HearthwardStorageSubsystem,world)
    state=json.loads(camp.describe());report['initial_camp']=state;flush()
    pos=state['camps'][0]['position'];center=unreal.Vector(pos['x'],pos['y'],pos['z'])
    player.set_actor_location(center+unreal.Vector(0,-150,100),False,True);brother.set_actor_location(center+unreal.Vector(-200,100,100),False,True)
    brother.camp.set_actor_location(center,False,True)
    check('seed wood source',camp.register_source('party-081-wood','wood',100,100,center+unreal.Vector(200,100,0),0))
    check('seed stone source',camp.register_source('party-081-stone','stone',100,100,center+unreal.Vector(200,150,0),0))
    epoch=store.get_timeline_epoch();check('occupied worker fixture',camp.assign_worker('camp_stone',0,epoch))
    ui.open_page('dialogue');yield delay(.3)
    def capture(label,w=1672,h=941):
        check('capture '+label,ui.capture_ui('TASK-081-'+label,w,h));(out/(label+'-layout.json')).write_text(ui.describe_layout(),encoding='utf-8')
    capture('home');capture('home-4x3',1024,768)
    components=json.loads(ui.describe_layout())['components'];visible=[x for x in components if 'action' in x]
    check('all dialogue elements stay on right',all(x['rect'][0]>=1030 for x in visible))
    check('no original background illustration',not any(x.get('id')=='background' for x in visible))
    check('gather row hit target',ui.action_at(unreal.Vector2D(1190,350))=='dialogue.gather')
    check('gather form',ui.execute_action('dialogue.gather'));capture('gather')
    check('16 unit gather proposed',ui.execute_action('dialogue.propose'));check('gather quantity exact',ai.get_candidate().quantity==16);capture('gather-card')
    check('back discards unconfirmed card',ui.execute_action('back') and not ai.has_candidate())
    check('team form',ui.execute_action('dialogue.team'));capture('team')
    check('team proposed',ui.execute_action('dialogue.propose'));check('party card is explicit',ai.get_candidate().quantity==2 and str(ai.get_candidate().intent)=='camp_team');capture('team-card')
    check('no allocation before confirmation',not next(r for r in json.loads(camp.describe())['regions'] if r['id']=='camp_wood')['brother'])
    check('confirm via visible action',ui.execute_action('agentConfirm:'+unreal.GuidLibrary.conv_guid_to_string(ai.get_candidate_id())))
    wood=lambda:next(r for r in json.loads(camp.describe())['regions'] if r['id']=='camp_wood')
    check('brother and two idle workers assigned',wood()['brother'] and len(wood()['workers'])==2 and 0 not in wood()['workers'] and wood()['enabled'])
    check('occupied worker untouched',next(r for r in json.loads(camp.describe())['regions'] if r['id']=='camp_stone')['workers']==[0])
    before=store.get_item_count('wood');yield lambda:store.get_item_count('wood')>before
    check('actual camp output enters storage',store.get_item_count('wood')>before)
    ui.execute_action('dialogue.status');capture('team-status')
    check('pause creates confirmation',ui.execute_action('dialogue.stopTeam') and ai.has_candidate())
    check('pause accepted',ai.confirm_candidate(ai.get_candidate_id()));check('pause retains assignment',not wood()['enabled'] and len(wood()['workers'])==2)
    before=store.get_item_count('wood');yield delay(2);check('paused queue produces nothing',store.get_item_count('wood')==before)
    # Real model requests, separate from deterministic UI calls.
    for text,expected in [('带两名族人一起采集木材','camp_team'),('现在采集队做得怎么样了？','task_status'),('暂停木材采集队','camp_team_stop')]:
        player.set_actor_location(brother.get_actor_location()+unreal.Vector(0,-100,0),False,True)
        check('model request '+text,ai.submit_player_text(player,brother,text));yield lambda:not ai.is_busy()
        raw=ai.get_last_structured_result();report['model_cases'].append({'input':text,'raw':raw,'line':ai.get_npc_line(),'status':ai.get_status()});flush()
        check('model intent '+text,json.loads(raw)['intent']==expected)
        if expected=='task_status':check('authoritative team feedback','2名族人' in ai.get_npc_line() and not ai.has_candidate())
        else:
            check('model creates pending card '+text,ai.has_candidate());check('confirm model proposal '+text,ai.confirm_candidate(ai.get_candidate_id()))
    check('stale preview staged',ai.set_structured_goal(player,brother,goal('camp_team','wood',2,'workers','current_camp')))
    stale=ai.get_candidate_id();brother.request(player,'invalidate prior request');check('stale confirmation rejected',not ai.confirm_candidate(stale))
    check('stale request did not resume',not wood()['enabled'])
    ui.execute_action('dialogue.home');ui.execute_action('dialogue.routes');capture('transport-routes')
    check('transport direction selected',ui.execute_action('dialogue.route:1'));capture('transport-form')
    report['ok']=True
runner=run();pending=None;deadline=time.monotonic()+900
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Dialogue081 PIE')
        if pending and not pending():return
        pending=next(runner);return
    except StopIteration:pass
    except Exception:report['error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
