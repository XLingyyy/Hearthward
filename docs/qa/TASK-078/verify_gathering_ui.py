"""Isolated PIE: real task-card actions, persistent blocked HUD and clan assignment.
Explicit fixture cargo/empty source supplies the 14/32 state; native test covers actual gathering/navigation.
"""
from pathlib import Path
import json,time,traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
card_only='-HearthwardCardOnly' in unreal.SystemLibrary.get_command_line()
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-078'/('card-visual' if card_only else 'pie')
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
    ui.open_page('dialogue')
    check('new manual task card',ui.execute_action('agentCollectCard'))
    for _ in range(31):
        check('adjust card quantity',ai.adjust_candidate(ai.get_candidate_id(),1))
    check('manual card requests 32', '32' in ai.get_candidate_text())
    check('capture source availability warning',ui.capture_ui('TASK-078-task-card',1672,941))
    if card_only:
        report['ok']=True
        return
    check('confirm manual task',ai.confirm_candidate(ai.get_candidate_id()))
    ui.open_page('hud')
    yield lambda:brother.get_phase()==unreal.HearthwardCompanionPhase.WAITING_AT_CAMP
    check('fourteen delivered before depletion',brother.get_delivered()==14 and brother.get_carried()==0)
    check('depletion explained', '资源不足' in str(brother.block_reason))
    yield delay(3)
    check('capture persistent HUD after transient lifetime',ui.capture_ui('TASK-078-blocked-hud',1672,941))
    ui.open_page('dialogue')
    check('empty-source retry refused',not ui.execute_action('agentRetryPath'))
    check('retry preserves delivered count',brother.get_delivered()==14)
    ui.open_page('building')
    check('camp management entry',ui.execute_action('page:camp'))
    check('worker tab',ui.execute_action('camp.tab:workers'))
    check('wood shortcut hit target',ui.action_at(unreal.Vector2D(700,248))=='camp.region:camp_wood')
    check('stone shortcut hit target',ui.action_at(unreal.Vector2D(920,248))=='camp.region:camp_stone')
    check('wood region selection',ui.execute_action('camp.region:camp_wood'))
    check('assign first clan worker',ui.execute_action('camp.assign'))
    check('start wood production',ui.execute_action('camp.toggle'))
    check('stone region selection',ui.execute_action('camp.region:camp_stone'))
    check('select second clan worker',ui.execute_action('camp.personNext'))
    check('assign second clan worker',ui.execute_action('camp.assign'))
    check('start stone production',ui.execute_action('camp.toggle'))
    state=json.loads(camp.describe())
    report['assigned_regions']=[r for r in state['regions'] if r['id'] in ('camp_wood','camp_stone')]
    check('both work regions enabled and assigned',len(report['assigned_regions'])==2 and all(r['enabled'] and len(r['workers'])==1 for r in report['assigned_regions']))
    check('capture clan assignment instructions',ui.capture_ui('TASK-078-clan-work',1672,941))
    report['ok']=True

runner=run();pending=None;deadline=time.monotonic()+240
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Gathering UI PIE')
        if pending and not pending():return
        pending=next(runner);return
    except StopIteration:pass
    except Exception:report['error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
