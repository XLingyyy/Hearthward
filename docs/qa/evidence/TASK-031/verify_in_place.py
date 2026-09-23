"""Regression: multiple locomotion loops must remain inside the capsule."""
import json,math,time,traceback
from pathlib import Path
import unreal

phase='after-final'
out=Path(unreal.Paths.project_saved_dir())/('HeroValidation/inplace-'+phase)
out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'stages':{}};state={};pending=None
def delay(s):
    end=time.monotonic()+s
    return lambda:time.monotonic()>=end
def flow():
    levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-100))
    floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(unreal.Vector(400,400,1))
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);p=unreal.GameplayStatics.get_player_pawn(world,0)
    pc.get_hud().screen.open_page('hud');unreal.GameplayStatics.set_game_paused(world,False)
    p.set_actor_location(unreal.Vector(-10000,-10000,100),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=0))
    actions=[a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==p]
    subs=[s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(s.get_outer(),unreal.LocalPlayer)]
    sub=next(s for s in subs if any(s.query_keys_mapped_to_action(a) for a in actions))
    move=next(a for a in actions if any(str(unreal.InputLibrary.key_get_display_name(k))=='W' for k in sub.query_keys_mapped_to_action(a)))
    sprint=next(a for a in actions if a.get_name()=='SprintAction')
    state.update(p=p,sub=sub,move=move,sprint=sprint,axis=unreal.Vector(),running=False,name=None)
    yield delay(2)
    for name,running in [('walk',False),('sprint',True)]:
        # Reset horizontally on the same floor; do not inject a fall/landing pose.
        p.set_actor_location(unreal.Vector(-10000,-10000,p.get_actor_location().z),False,True)
        state.update(axis=unreal.Vector(0,1,0),running=running)
        end=time.monotonic()+5
        yield lambda:(p.get_velocity().length()>(500 if running else 300) and str(p.mesh.get_anim_instance().motion_state)==('Sprint' if running else 'Walk')) or time.monotonic()>end
        yield delay(.3)
        state['name']=name;report['stages'][name]=[]
        yield delay(9)
        unreal.SystemLibrary.execute_console_command(world,'HighResShot 1024x768 filename="'+str(out/(name+'.png'))+'"',pc)
        yield delay(.3)
        state['name']=None;state['axis']=unreal.Vector();state['running']=False
        p.character_movement.stop_movement_immediately();yield delay(.5)
    for name,values in report['stages'].items():
        radius=max(v['hip_radius_cm'] for v in values)
        jump=max(math.dist(a['hip_component'],b['hip_component']) for a,b in zip(values,values[1:]))
        report['checks'][name+'_hip_stays_in_capsule']=radius<30
        report['checks'][name+'_no_loop_snap']=jump<12
        report['checks'][name+'_actual_motion']=sum(v['speed']>(400 if name=='sprint' else 150) for v in values)/len(values)>.95
        report[name]={'max_hip_radius_cm':radius,'max_frame_hip_delta_cm':jump,'samples':len(values)}
    report['ok']=all(report['checks'].values())
iterator=flow()
def tick(dt):
    global pending
    try:
        if state:
            p=state['p'];state['sub'].inject_input_vector_for_action(state['move'],state['axis'],[],[])
            state['sub'].inject_input_vector_for_action(state['sprint'],unreal.Vector(1 if state['running'] else 0,0,0),[],[])
            if state['name']:
                hip=p.mesh.get_socket_location('pelvis');loc=p.get_actor_location()
                c=p.mesh.get_socket_transform('pelvis',unreal.RelativeTransformSpace.RTS_COMPONENT).translation
                cam=p.get_component_by_class(unreal.CameraComponent).get_world_location()
                report['stages'][state['name']].append({'hip_radius_cm':math.hypot(hip.x-loc.x,hip.y-loc.y),'hip_component':[c.x,c.y,c.z],
                    'actor':[loc.x,loc.y,loc.z],'camera':[cam.x,cam.y,cam.z],'speed':p.get_velocity().length()})
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:finish()
    except Exception:report['error']=traceback.format_exc();finish()
def finish():
    (out/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
