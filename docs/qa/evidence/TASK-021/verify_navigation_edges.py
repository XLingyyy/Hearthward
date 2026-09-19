"""Real PIE navigation, inventory settlement, cancellation and save restoration."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task021';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'checks':{},'positions':[],'scope':'isolated save pool, unsaved temporary level obstacles, native AI navigation'}
state={};levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=35):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+5)
def place(actor,x,y,z=90):actor.set_actor_location(unreal.Vector(x,y,z),False,True)
def phase():return state['companion'].get_phase()
def capture(name):
    unreal.SystemLibrary.execute_console_command(state['world'],'HighResShot 1280x720 filename="'+(out/(name+'.png')).as_posix()+'"',state['pc'])
def finish():
    (out/'edges-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube=unreal.load_asset('/Engine/BasicShapes/Cube')
    # Unsaved editor actors are copied into PIE; no Content assets are modified.
    for i,(x,y,sx,sy) in enumerate([(550,-700,1,5),(950,-700,.6,4),(1250,-700,.6,4),(1100,-900,3.6,.6),(1100,-500,3.6,.6)]):
        a=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,150))
        a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        a.static_mesh_component.set_static_mesh(cube)
        a.set_actor_scale3d(unreal.Vector(sx,sy,3));a.tags=['Task021.Wall'+str(i)]
        a.static_mesh_component.set_collision_profile_name('BlockAll')
        if i>0:a.set_actor_location(unreal.Vector(x,y,-2000),False,True)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('new_campaign',ui.execute_action('new'));yield delay(1)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    stock=next(s for s in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if s.get_outer()==world)
    saves=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    state.update(world=world,pc=pc,companion=companion)
    walls=[unreal.GameplayStatics.get_all_actors_with_tag(world,'Task021.Wall'+str(i))[0] for i in range(5)]
    camp=companion.camp;source=companion.source.get_owner()
    place(companion,100,-700);place(camp,100,-700);place(source,1100,-700)
    place(player,100,-100);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=-25))
    yield delay(2)
    initial=stock.get_item_count('wood')
    ticket=companion.request(player,'blocked return cargo')
    check('collect_accepted',companion.submit(player,ticket,'wood',4,['collect','return','deposit'])==unreal.HearthwardProposalResult.ACCEPTED)
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.RETURNING,35)
    for a in walls[1:]:
        p=a.get_actor_location();place(a,p.x,p.y,150)
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.RETURNING_BLOCKED,15)
    yield delay(1)
    check('blocked_cargo_retained',companion.bag.get_item_count('wood')==4 and stock.get_item_count('wood')==initial)
    check('blocked_reason_visible',bool(companion.block_reason))
    for a in walls[1:]:
        p=a.get_actor_location();place(a,p.x,p.y,-2000)
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.COMPLETED,25)
    check('opened_route_resumes_delivery',companion.bag.get_item_count('wood')==0 and stock.get_item_count('wood')==initial+4)
    check('completed_clears_block_reason',not companion.block_reason)
    # Existing encounter actors are recognized by their actual configured spawn locations.
    origin=unreal.Vector(200,200,-10)
    actors=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor)
    targets=[]
    for x,y in [(1200,-400),(1600,-300),(1600,-800)]:
        targets.append(min(actors,key=lambda a:(a.get_actor_location()-unreal.Vector(x,y,90)).length()))
    check('three_encounter_actors',len(set(targets))==3 and all(a!=player and a!=companion for a in targets))
    walls[0].set_actor_scale3d(unreal.Vector(.2,5,3));yield delay(2)
    place(targets[0],600,-700);place(targets[1],4000,2000);place(targets[2],4200,2000)
    place(companion,500,-700);place(player,100,-100)
    before=sum(game.opponents.values())
    check('attack_order',game.order_companion('attack'))
    yield delay(.25)
    check('no_attack_through_wall',sum(game.opponents.values())==before)
    state['record']=True
    yield wait(lambda:sum(game.opponents.values())<before,25)
    state['record']=False
    check('attack_detours_then_damages',companion.get_actor_location().x>570 and any(abs(p[1]+700)>250 for p in report['positions']))
    capture('attack-around-wall');yield delay(.2)
    check('wait_after_attack',game.order_companion('wait'))
    stopped=companion.get_actor_location();yield delay(.5)
    check('wait_stops_navigation',(companion.get_actor_location()-stopped).length()<2)
    # Moving follow target changes the active destination, without a new user command.
    place(companion,100,-700);place(player,1100,-700)
    game.order_companion('follow');yield delay(1)
    place(player,100,-1200)
    yield wait(lambda:(companion.get_actor_location()-player.get_actor_location()).length()<180,20)
    check('moving_target_repaths',companion.get_actor_location().y<-1000)
    game.order_companion('wait')
    report['passed']=True;finish()
iterator=run();pending=None;deadline=time.monotonic()+230
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('navigation overall')
        if state.get('record'):
            p=state['companion'].get_actor_location();report['positions'].append([round(p.x,2),round(p.y,2),round(p.z,2)])
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('navigation stage; checks='+str(report['checks']))
            if not pending[0]():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
