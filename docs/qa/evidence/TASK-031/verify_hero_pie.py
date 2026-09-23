"""PIE motion checks and rendered evidence using an isolated development fixture."""
import json
import math
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
interactive='-Task027Interactive' in unreal.SystemLibrary.get_command_line()
out=Path(unreal.Paths.project_saved_dir())/'HeroValidation/playtest5'
out.mkdir(parents=True,exist_ok=True)
(out/'frames').mkdir(exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'samples':[], 'input':('Physical keyboard/mouse observation; passed means observer completed' if interactive else 'Enhanced Input injection and gameplay API')}
stage='setup'; active={}; frame=0; last_capture=0.; pending_capture=False

def check(name,value):
    report['checks'][name]=bool(value)
    if not value: raise AssertionError(name)

def delay(seconds):
    deadline=time.monotonic()+seconds
    return lambda:time.monotonic()>=deadline

def run():
    global stage,active
    levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for cls,label in [(unreal.SceneCapture2D,'Task027Capture'),(unreal.PointLight,'Task027Light')]:
        actor=actors.spawn_actor_from_class(cls,unreal.Vector(200,-200,350))
        actor.set_editor_property('tags',[label])
        actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    pc.get_hud().screen.open_page('hud')
    unreal.GameplayStatics.set_game_paused(world,False)
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc)
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    game.enable_adventure()
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    bag.try_add('axe',1); game.equip('axe')
    companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    companion.set_actor_location(unreal.Vector(-800,-800,100),False,True)
    player.set_actor_location(unreal.Vector(0,0,100),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=0))
    anim=player.mesh.get_anim_instance()
    check('native_animation_instance',isinstance(anim,unreal.HearthwardHeroAnimInstance))
    clips=anim.get_editor_property('clips')
    check('eight_loaded_clips',len(clips)==8 and all(clips))
    check('real_hero_mesh',player.mesh.skeletal_mesh_asset.get_name()=='SK_Hero')
    actions=[a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==player]
    subs=[s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(s.get_outer(),unreal.LocalPlayer)]
    sub=next(s for s in subs if any(s.query_keys_mapped_to_action(a) for a in actions))
    move=next(a for a in actions if any(str(unreal.InputLibrary.key_get_display_name(k))=='W' for k in sub.query_keys_mapped_to_action(a)))
    sprint=next(a for a in actions if a.get_name()=='SprintAction')
    capture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('Task027Capture'))
    target=unreal.RenderingLibrary.create_render_target2d(world,960,720,unreal.TextureRenderTargetFormat.RTF_RGBA8)
    capture.capture_component2d.texture_target=target
    capture.capture_component2d.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
    capture.capture_component2d.set_editor_property('fov_angle',45)
    capture.capture_component2d.set_editor_property('capture_every_frame',False)
    capture.capture_component2d.set_editor_property('capture_on_movement',False)
    light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('Task027Light'))
    light.point_light_component.set_intensity(150)
    light.point_light_component.set_attenuation_radius(1500)
    light.point_light_component.set_cast_shadows(False)
    active={'world':world,'pc':pc,'player':player,'anim':anim,'sub':sub,'move':move,'axis':unreal.Vector(),'sprint':sprint,'sprinting':False,'capture':capture,'target':target,'light':light}
    if interactive:
        game.set_editor_property('enabled',False)
        companion.source.get_owner().set_actor_location(unreal.Vector(120,0,100),False,True)
        stage='physical_input'
        marker=Path(unreal.Paths.project_dir())/'.agent-local/task027-stop-physical'
        while not marker.exists():
            yield delay(.1)
        report['observed_states']=sorted({s['motion'] for s in report['samples']})
        report['passed']=True
        return
    stage='idle';yield delay(1)
    check('idle_state',str(anim.motion_state)=='Idle')
    stage='walk';active['axis']=unreal.Vector(0,1,0);yield delay(1)
    check('walking_state_and_speed',str(anim.motion_state)=='Walk' and anim.ground_speed>150)
    foot_height=min(player.mesh.get_socket_location(b).z for b in ['foot_l','foot_r'])-(player.get_actor_location().z-90.)
    report['walking_ankle_height_above_capsule_base']=foot_height
    check('walking_support_foot_height',-5.<foot_height<20.)
    walk_pose=player.mesh.get_socket_transform('calf_l',unreal.RelativeTransformSpace.RTS_COMPONENT)
    yield delay(.18)
    later=player.mesh.get_socket_transform('calf_l',unreal.RelativeTransformSpace.RTS_COMPONENT)
    check('walking_joint_actually_moves',walk_pose.translation.distance(later.translation)>.1)
    active['axis']=unreal.Vector();player.set_actor_location(unreal.Vector(0,0,100),False,True)
    game.set_sprinting(True);stage='sprint';active['axis']=unreal.Vector(0,1,0);yield delay(1)
    check('sprint_state_and_speed',str(anim.motion_state)=='Sprint' and anim.ground_speed>400)
    active['axis']=unreal.Vector();game.set_sprinting(False);player.character_movement.stop_movement_immediately();yield delay(.3)
    player.set_actor_location(unreal.Vector(0,0,100),False,True);yield delay(.3)
    stage='jump';player.jump();yield delay(.1)
    check('jump_start_state',str(anim.motion_state)=='JumpStart')
    player.stop_jumping();end=time.monotonic()+5
    yield lambda:str(anim.motion_state)=='Fall' or time.monotonic()>end
    check('fall_state',str(anim.motion_state)=='Fall')
    end=time.monotonic()+5
    yield lambda:not player.character_movement.is_falling() and str(anim.motion_state)=='Idle' or time.monotonic()>end
    check('landed_and_returned_idle',not player.character_movement.is_falling() and str(anim.motion_state)=='Idle')
    stage='dig';source=companion.source
    source.get_owner().set_actor_location(unreal.Vector(120,0,100),False,True)
    player.set_actor_rotation(unreal.Rotator(),False)
    player.set_actor_location(source.get_owner().get_actor_location()+unreal.Vector(0,90,0),False,True)
    yield delay(.4)
    interaction=player.get_component_by_class(unreal.HearthwardInteractionComponent)
    check('resource_interaction_starts',interaction.interact_nearest());yield delay(.4)
    check('dig_state_during_real_action',str(anim.motion_state)=='Dig')
    timer=player.get_component_by_class(unreal.HearthwardTimedActionComponent)
    stage='dig_pause';unreal.GameplayStatics.set_game_paused(world,True);before=timer.get_elapsed_seconds();yield delay(.4)
    check('paused_action_does_not_advance',abs(timer.get_elapsed_seconds()-before)<.01)
    unreal.GameplayStatics.set_game_paused(world,False);stage='dig';yield delay(.8)
    active['axis']=unreal.Vector(1,0,0);yield delay(.3)
    check('movement_cancels_dig',str(anim.motion_state)!='Dig' and interaction.get_status()!=unreal.HearthwardInteractionStatus.RUNNING)
    active['axis']=unreal.Vector();player.character_movement.stop_movement_immediately()
    player.set_actor_location(source.get_owner().get_actor_location()+unreal.Vector(0,90,0),False,True);yield delay(.3)
    before=bag.get_item_count('wood');target=source.get_owner().get_component_by_class(unreal.HearthwardResourceInteractionComponent)
    report['second_target']=target.get_path_name()
    check('second_interaction_starts',interaction.begin_interaction(target));stage='dig_complete'
    end=time.monotonic()+10
    yield lambda:interaction.get_status()!=unreal.HearthwardInteractionStatus.RUNNING or time.monotonic()>end
    yield delay(.2)
    report['resource_result']={'before':before,'after':bag.get_item_count('wood'),'status':str(interaction.get_status()),'feedback':interaction.get_completion_feedback(),'motion':str(anim.motion_state)}
    check('resource_settles_once_and_animation_exits',bag.get_item_count('wood')==before+1 and str(anim.motion_state)=='Idle')
    origin=companion.camp.get_actor_location()-unreal.Vector(0,0,100)
    player.set_actor_location(origin+unreal.Vector(900,-600,100),False,True);yield delay(.3)
    stage='attack';before=game.opponents.get('guard_1')
    check('real_attack_accepted',game.attack());yield delay(.15)
    check('attack_animation_and_damage',str(anim.motion_state)=='Attack' and game.opponents.get('guard_1')<before)
    yield delay(.52)
    check('next_attack_accepted_during_previous_recovery',game.attack())
    player.set_actor_location(unreal.Vector(0,0,100),False,True)
    player.set_actor_rotation(unreal.Rotator(),False)
    # Observe completion with a bound; wall-clock .7s can precede the final animation tick.
    attack_deadline=time.monotonic()+1.5
    yield lambda:str(anim.motion_state)=='Idle' or time.monotonic()>=attack_deadline
    check('attack_returns_to_locomotion',str(anim.motion_state)=='Idle')
    companion.set_actor_location(origin+unreal.Vector(900,-500,100),False,True)
    player.set_actor_location(origin+unreal.Vector(900,-700,100),False,True)
    before=game.opponents.get('guard_1');check('companion_assist_order',game.order_companion('attack'))
    end=time.monotonic()+5
    yield lambda:str(companion.mesh.get_anim_instance().motion_state)=='Attack' or time.monotonic()>end
    check('companion_attack_animation_and_damage',str(companion.mesh.get_anim_instance().motion_state)=='Attack' and game.opponents.get('guard_1')<before)
    game.order_companion('wait');player.set_actor_location(unreal.Vector(0,0,100),False,True)
    stage='natural_sprint';game.set_editor_property('enabled',False)
    active['sprinting']=True;active['axis']=unreal.Vector(0,1,0);yield delay(.8)
    check('natural_mode_sprint_input_binding',str(anim.motion_state)=='Sprint' and anim.ground_speed>400)
    active['sprinting']=False;yield delay(.4)
    check('release_sprint_restores_walk_speed',str(anim.motion_state)=='Walk' and anim.ground_speed<360)
    active['axis']=unreal.Vector();game.set_editor_property('enabled',True)
    stage='dead';game.set_editor_property('health',0);yield delay(.3)
    check('death_clears_action_animation',str(anim.motion_state)=='Idle')
    report['passed']=True

iterator=run();pending=None;deadline=time.monotonic()+(600 if interactive else 100)
def tick(delta):
    global pending,frame,last_capture,pending_capture
    try:
        now=time.monotonic()
        if now>deadline: raise TimeoutError(stage)
        if active:
            p=active['player'];a=active['anim'];c=active['capture'];w=active['world'];loc=p.get_actor_location()
            if not interactive:
                active['sub'].inject_input_vector_for_action(active['move'],active['axis'],[],[])
                active['sub'].inject_input_vector_for_action(active['sprint'],unreal.Vector(1 if active['sprinting'] else 0,0,0),[],[])
            if pending_capture:
                unreal.RenderingLibrary.export_render_target(w,active['target'],str(out/'frames'),f'{frame:04d}.png')
                frame+=1;pending_capture=False
            if now-last_capture>=1/6:
                last_capture=now
                camera=loc+unreal.Vector(310,-290,100)
                c.set_actor_location(camera,False,True)
                c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,loc),False)
                active['light'].set_actor_location(loc+unreal.Vector(180,-160,240),False,True)
                c.capture_component2d.capture_scene();pending_capture=True
                report['samples'].append({'frame':frame,'stage':stage,'motion':str(a.motion_state),'speed':a.ground_speed,'location':[loc.x,loc.y,loc.z]})
                if interactive:
                    (out/'live.json').write_text(json.dumps(report['samples'][-1]),encoding='utf-8')
        if pending and not pending(): return
        pending=next(iterator)
    except StopIteration: finish()
    except Exception:
        report['error']=traceback.format_exc();finish()

def finish():
    report['ok']=report['passed'] and not report.get('error')
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
    unreal.SystemLibrary.quit_editor()

handle=unreal.register_slate_post_tick_callback(tick)
