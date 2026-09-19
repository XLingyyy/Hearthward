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
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
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
    check('native_ai_controller',isinstance(companion.get_controller(),unreal.AIController))
    check('character_movement_grounded',companion.character_movement.is_moving_on_ground())
    initial=stock.get_item_count('wood');resource=companion.source.get_item_count('wood')
    ticket=companion.request(player,'navigation regression collect eight wood')
    check('accept_multitrip',companion.submit(player,ticket,'wood',8,['collect','return','deposit'])==unreal.HearthwardProposalResult.ACCEPTED)
    state['record']=True
    yield wait(lambda:companion.get_delivered()>=4,50)
    capture('around-wall-delivery');yield delay(.2)
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.COMPLETED,50)
    state['record']=False
    check('eight_delivered',stock.get_item_count('wood')==initial+8 and companion.get_delivered()==8)
    check('resource_conserved',companion.source.get_item_count('wood')==resource-8 and companion.bag.get_item_count('wood')==0)
    check('detour_observed',any(abs(p[1]+700)>250 for p in report['positions']))
    check('never_crossed_wall',all(not (470<p[0]<630 and -970<p[1]<-430) for p in report['positions']))
    # Cancel an in-flight route; no movement or settlement may continue.
    t=companion.request(player,'cancel route');companion.submit(player,t,'wood',4,['collect','return','deposit'])
    yield delay(.6);check('cancel_accepted',companion.cancel(player));p=companion.get_actor_location();yield delay(.6)
    check('cancel_stops_movement',(companion.get_actor_location()-p).length()<2)
    # Follow must use the same detour, then wait must stop it immediately.
    place(companion,100,-700);place(player,1100,-700)
    check('follow_order',game.order_companion('follow'));yield wait(lambda:(companion.get_actor_location()-player.get_actor_location()).length()<210,25)
    check('follow_reaches_player',companion.get_actor_location().x>800)
    game.order_companion('wait');yield delay(.2)
    # Snapshot while returning with cargo; loading invalidates the future navigation request.
    place(companion,100,-700);place(player,100,-100)
    t=companion.request(player,'saved route');companion.submit(player,t,'wood',4,['collect','return','deposit'])
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.RETURNING,30)
    check('carrying_real_wood',companion.bag.get_item_count('wood')==4)
    check('save_during_return',saves.save_point(True));point=saves.get_points()[-1].save_id
    saved_position=companion.get_actor_location();saved_stock=stock.get_item_count('wood')
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.COMPLETED,25)
    check('load_return_snapshot',saves.load_point(point));ui.open_page('hud')
    check('restore_position',(companion.get_actor_location()-saved_position).length()<2)
    check('restore_cargo',companion.bag.get_item_count('wood')==4 and stock.get_item_count('wood')==saved_stock)
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.COMPLETED,25)
    check('restored_delivery_once',stock.get_item_count('wood')==saved_stock+4 and companion.bag.get_item_count('wood')==0)
    # Enclose the resource. Partial routes must not be mistaken for a reachable destination.
    for i,a in enumerate(walls[1:]):
        p=a.get_actor_location();place(a,p.x,p.y,150)
    yield delay(2)
    t=companion.request(player,'unreachable resource');companion.submit(player,t,'wood',4,['collect','return','deposit'])
    yield wait(lambda:phase()==unreal.HearthwardCompanionPhase.WAITING_AT_CAMP,15)
    check('unreachable_returns_waiting',bool(companion.block_reason))
    check('unreachable_no_fake_delivery',companion.get_delivered()==0 and stock.get_item_count('wood')==saved_stock+4)
    capture('unreachable-waits-at-camp');yield delay(.3)
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
