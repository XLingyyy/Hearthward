"""Isolated PIE: real local-model status/resume requests and UE confirmation/settlement.
Explicit cargo and source fixtures seed 14/32; replenishment is test setup, not an AI action.
"""
from pathlib import Path
import json,time,traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
card_only=False
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-079'/'pie'
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'method':__doc__}

def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)

def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end

def subsystem(cls,world):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)

def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(1600,1600,1))
    floor.static_mesh_component.set_collision_profile_name('BlockAll')
    floor.tags=['Hearthward.NatureGround']
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    ui=pc.get_hud().get_editor_property('screen')
    unreal.GameplayStatics.set_game_paused(world,False)
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc)
    yield delay(.5)
    brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    game.enable_adventure()
    game.order_companion('wait')
    game.set_component_tick_enabled(False)
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
        if actor.get_component_by_class(unreal.HearthwardCombatTargetComponent):
            actor.set_actor_location(unreal.Vector(150000,150000,500),False,True)
    save=subsystem(unreal.HearthwardSaveSubsystem,world)
    check('isolated prototype enabled',save.enable_prototype())
    check('isolated new progress',save.start_new_progress())
    ai=subsystem(unreal.HearthwardLocalAISubsystem,world)
    camp=subsystem(unreal.HearthwardCampSubsystem,world)
    store=subsystem(unreal.HearthwardStorageSubsystem,world)
    player.set_actor_location(unreal.Vector(-150,-150,100),False,True)
    brother.set_actor_location(unreal.Vector(-200,100,100),False,True)
    brother.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    brother.source.try_remove('wood',brother.source.get_item_count('wood'))
    check('fixture retained physical cargo',brother.bag.try_add('wood',14)==unreal.HearthwardInventoryResult.SUCCESS)
    check('fixture actual gathering tool',brother.bag.try_add('axe',1)==unreal.HearthwardInventoryResult.SUCCESS)
    ui.open_page('dialogue')
    check('fixture task card',ui.execute_action('agentCollectCard'))
    for _ in range(31):
        check('fixture requested quantity',ai.adjust_candidate(ai.get_candidate_id(),1))
    check('fixture task confirmed',ai.confirm_candidate(ai.get_candidate_id()))
    yield lambda:brother.get_phase()==unreal.HearthwardCompanionPhase.WAITING_AT_CAMP
    check('fixture actual delivery',brother.get_delivered()==14)
    command=unreal.GuidLibrary.conv_guid_to_string(brother.get_command_id())
    report['model_cases']=[]
    for text,expected in [('还差多少木头，为什么停下来了？','task_status'),
                          ('先不要继续，我只是问刚才的任务还差多少','task_status'),
                          ('继续刚才没完成的采集任务','resume')]:
        check('model request accepted: '+text,ai.submit_player_text(player,brother,text))
        yield lambda:not ai.is_busy()
        raw=ai.get_last_structured_result()
        row={'text':text,'raw':raw,'reply':ai.get_npc_line(),'reason':ai.get_reason_code(),'candidate':ai.has_candidate()}
        report['model_cases'].append(row)
        check('capture reply',ui.capture_ui('TASK-079-'+expected,1672,941))
        parsed=json.loads(raw)
        (out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        check('raw intent: '+text,parsed['intent']==expected)
        check('query keeps command: '+text,unreal.GuidLibrary.conv_guid_to_string(brother.get_command_id())==command and brother.get_delivered()==14)
        if expected=='task_status':
            check('actual counts in reply: '+text,all(x in ai.get_npc_line() for x in ('14','32','18')))
            check('query is read-only: '+text,not ai.has_candidate())
        else:
            check('resume needs confirmation',ai.has_candidate())
            check('resume card preserves task', '将替换任务' not in ai.get_candidate_text())
            check('empty source confirmation rejected',not ai.confirm_candidate(ai.get_candidate_id()))
            check('empty source explained','资源不足' in ai.get_npc_line())
            check('failed retry retains delivery',brother.get_delivered()==14 and unreal.GuidLibrary.conv_guid_to_string(brother.get_command_id())==command)
            # Explicit fixture replenishment; gameplay still performs gathering and settlement.
            brother.source.try_add('wood',18)
            brother.source.get_owner().set_actor_location(brother.get_actor_location(),False,True)
            check('resume original task',ai.confirm_candidate(ai.get_candidate_id()))
            check('resume retains command ID',unreal.GuidLibrary.conv_guid_to_string(brother.get_command_id())==command)
            yield lambda:brother.get_phase() in (unreal.HearthwardCompanionPhase.COMPLETED,unreal.HearthwardCompanionPhase.WAITING_AT_CAMP,unreal.HearthwardCompanionPhase.HOLDING_SAFELY)
            report['resumed_execution']={'phase':str(brother.get_phase()),'reason':brother.block_reason,'delivered':brother.get_delivered(),'source':brother.source.get_item_count('wood')}
            check('task reaches original 32',brother.get_delivered()==32)
    check('completed request accepted',ai.submit_player_text(player,brother,'刚才的任务做完了吗？'))
    yield lambda:not ai.is_busy()
    raw=ai.get_last_structured_result()
    report['model_cases'].append({'text':'刚才的任务做完了吗？','raw':raw,'reply':ai.get_npc_line()})
    check('completed status classified',json.loads(raw)['intent']=='task_status')
    check('completed status accurate','已完成' in ai.get_npc_line() and '32 / 32' in ai.get_npc_line())
    resume=unreal.HearthwardAgentGoal()
    for key,value in {'intent':'resume','item':'none','quantity':1,'quantity_mode':'directive','source_ref':'current_task'}.items():
        resume.set_editor_property(key,value)
    check('completed task cannot resume',not ai.set_structured_goal(player,brother,resume))
    # Deterministic confirmation boundary, counted separately from real language cases.
    brother.bag.try_add('wood',2)
    check('boundary seed card',ui.execute_action('agentCollectCard'))
    for _ in range(4):check('boundary quantity',ai.adjust_candidate(ai.get_candidate_id(),1))
    check('boundary seed accepted',ai.confirm_candidate(ai.get_candidate_id()))
    yield lambda:brother.get_phase()==unreal.HearthwardCompanionPhase.WAITING_AT_CAMP
    check('boundary resume card',ai.set_structured_goal(player,brother,resume))
    stale=ai.get_candidate_id()
    brother.request(player,'another request invalidates prior proposal')
    check('stale resume rejected',not ai.confirm_candidate(stale))
    check('stale card preserves progress',brother.get_delivered()==2 and brother.get_phase()==unreal.HearthwardCompanionPhase.WAITING_AT_CAMP)
    report['ok']=True

runner=run();pending=None;deadline=time.monotonic()+1200
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Companion dialogue PIE')
        if pending and not pending():return
        pending=next(runner);return
    except StopIteration:pass
    except Exception:report['error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
