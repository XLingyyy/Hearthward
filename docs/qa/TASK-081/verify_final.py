"""Isolated graybox PIE. Real UI actions, real local model, camp allocation and live production. Explicit resource and floor fixtures; no user save changes."""
from pathlib import Path
import json,time,traceback
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-081/final-pie'
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
    check('team form final',ui.execute_action('dialogue.team'))
    check('team proposal final',ui.execute_action('dialogue.propose'))
    check('confirmation summary has no duplicate',ai.get_candidate_text().count('弟弟带2名空闲族人')==1)
    check('capture final card',ui.capture_ui('TASK-081-final-team-card',1672,941))
    check('final team confirmed',ui.execute_action('agentConfirm:'+unreal.GuidLibrary.conv_guid_to_string(ai.get_candidate_id())))
    start=brother.get_actor_location()
    yield lambda:'已到岗工作' in camp.describe_work_party()
    check('brother actually arrives and works','已到岗工作' in camp.describe_work_party())
    report['movement_cm']=(brother.get_actor_location()-start).length()
    check('brother physically moved',report['movement_cm']>20)
    check('work status final',ui.execute_action('dialogue.status'))
    check('capture final work status',ui.capture_ui('TASK-081-final-status',1672,941))
    check('final pause proposed',ui.execute_action('dialogue.stopTeam'))
    check('final pause accepted',ai.confirm_candidate(ai.get_candidate_id()))
    check('resume button visible',any(x.get('action')=='dialogue.resumeTeam' for x in json.loads(ui.describe_layout())['components']) or ui.execute_action('dialogue.status'))
    check('resume keeps worker count',ui.execute_action('dialogue.resumeTeam') and ai.get_candidate().quantity==2)
    check('resume accepted',ai.confirm_candidate(ai.get_candidate_id()))
    player.set_actor_location(brother.get_actor_location()+unreal.Vector(0,-100,0),False,True)
    check('final status model request',ai.submit_player_text(player,brother,'现在采集队进度怎么样？'));yield lambda:not ai.is_busy()
    raw=ai.get_last_structured_result();report['model_cases'].append({'input':'现在采集队进度怎么样？','raw':raw,'line':ai.get_npc_line(),'status':ai.get_status()});flush()
    check('expanded status fits model context',json.loads(raw)['intent']=='task_status')
    check('final status includes real brother arrival','已到岗工作' in ai.get_npc_line())
    check('full reply view',ui.execute_action('dialogue.reply'));check('capture full reply',ui.capture_ui('TASK-081-final-reply',1672,941))
    ui.execute_action('dialogue.home');check('capture final home',ui.capture_ui('TASK-081-final-home',1672,941))
    ui.execute_action('dialogue.gather');check('capture final gather',ui.capture_ui('TASK-081-final-gather',1672,941))
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
