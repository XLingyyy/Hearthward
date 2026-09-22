"""Real PIE movement and scene evidence for the rebuilt natural map.

Execute in the UE editor after authoring and saving. Route movement uses the
existing Enhanced Input action at the character's unchanged walking speed.
No teleport is used during a route. Observation-only teleports are labelled.
"""
from pathlib import Path
import json
import time
import math
import traceback
import unreal

ROOT=Path(unreal.Paths.project_dir())
OUT=ROOT/'Saved/Task026/Rebuild/verification'
OUT.mkdir(parents=True,exist_ok=True)
SRC=ROOT/'art_source/TASK-026/Rebuild'
MAP='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
config_path=SRC/'verification_options.json'
options=json.loads(config_path.read_text()) if config_path.exists() else {'route_seconds':90}
routes=json.loads((SRC/'routes.json').read_text())
report={'passed':False,'map':MAP,'checks':{},'samples':[],'captures':[],
        'route_complete':False,'input_method':'Existing Enhanced Input, default 350 cm/s, no route teleport',
        'scope':'PIE; not Standalone or final target-hardware performance acceptance'}
state={'forward':False,'route':False,'frames':[]}
unreal.EditorPythonScripting.set_keep_python_script_alive(True)


def check(key,value):
    report['checks'][key]=bool(value)
    if not value:raise AssertionError(key)


def wait_for(predicate,seconds=60):return predicate,time.monotonic()+seconds


def delay(seconds):
    until=time.monotonic()+seconds
    return wait_for(lambda:time.monotonic()>until,seconds+30)


def capture(name,width=1280,height=720):
    p=OUT/(name+'.png')
    unreal.SystemLibrary.execute_console_command(state['world'],f'HighResShot {width}x{height} filename="{p.as_posix()}"',state['pc'])
    report['captures'].append(name+'.png')


def run():
    check('map_reopens',levels.load_level(MAP))
    levels.editor_request_begin_play()
    yield wait_for(levels.is_in_play_in_editor)
    state['world']=editor.get_game_world()
    yield wait_for(lambda:bool(unreal.GameplayStatics.get_player_pawn(state['world'],0)))
    world=state['world'];pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    state.update(pawn=pawn,pc=pc)
    yield delay(8)
    actions=sorted([a for a in unreal.ObjectIterator(unreal.InputAction)
                    if a.get_outer()==pawn and a.value_type==unreal.InputActionValueType.AXIS2D],key=lambda a:a.get_name())
    check('native_move_actions',len(actions)==2)
    subsystems=[s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
                if isinstance(s.get_outer(),unreal.LocalPlayer) and s.query_keys_mapped_to_action(actions[0])]
    check('single_input_owner',len(subsystems)==1)
    state.update(input=subsystems[0],action=actions[0])
    check('default_walking_speed',abs(pawn.character_movement.max_walk_speed-350)<1)
    p=pawn.get_actor_location();start=routes['loop'][0]
    report['spawn_cm']=[p.x,p.y,p.z]
    check('spawn_ground_height',abs(p.z-(start[2]*100+96))<120)
    check('spawn_not_falling',pawn.character_movement.is_moving_on_ground())
    local_ai=next(s for s in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if s.get_outer()==world)
    storage=next(s for s in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if s.get_outer()==world)
    check('no_local_model',local_ai.get_server_process_id()==0)
    check('empty_gameplay_storage',storage.get_item_count('wood')==0)
    check('no_companion_fixture',not unreal.GameplayStatics.get_all_actors_with_tag(world,'Hearthward.Companion.PROTOTYPE_ONLY'))
    check('natural_batches_streamed',len(unreal.GameplayStatics.get_all_actors_with_tag(world,'TASK026.REBUILD'))>0)
    report['runtime_grass_components']=[c.get_instance_count() for c in unreal.ObjectIterator(unreal.GrassInstancedStaticMeshComponent) if c.get_owner() and c.get_owner().get_world()==world]
    capture('spawn')
    yield delay(2)
    state.update(route=True,route_index=1,route_start=time.monotonic(),last_sample=0,last_capture=0,
                 progress_at=time.monotonic(),progress_location=p,walked_cm=0,last_location=p,
                 path=routes['loop'],route_name='main',route_limit=options.get('route_seconds',90),route_done=False)
    yield wait_for(lambda: not state['route'],options.get('route_seconds',90)+90)
    check('walked_at_least_100m',state['walked_cm']>=10000)
    report['route_walked_m']=state['walked_cm']/100
    report['route_seconds']=time.monotonic()-state['route_start']
    capture('walk-end')
    yield delay(2)
    # Fixed views are separate from the route validation, and disclose teleport.
    for name,x,y,z,yaw in options.get('observations',[]):
        state['pawn'].character_movement.stop_movement_immediately()
        state['pawn'].set_actor_location(unreal.Vector(x*100,y*100,z*100+150),False,True)
        state['pc'].set_control_rotation(unreal.Rotator(pitch=-8,yaw=yaw,roll=0))
        yield delay(8)
        report.setdefault('observation_teleports',[]).append([name,x,y,z])
        capture(name)
        yield delay(2)
    for name,path in options.get('crossings',[]):
        x,y,z=path[0]
        pawn.character_movement.stop_movement_immediately()
        pawn.set_actor_location(unreal.Vector(x*100,y*100,z*100+150),False,True)
        yield delay(8)
        p=pawn.get_actor_location()
        report.setdefault('crossing_setup_teleports',[]).append([name,x,y,z])
        state.update(route=True,route_index=1,route_start=time.monotonic(),last_sample=0,last_capture=0,
                     progress_at=time.monotonic(),progress_location=p,walked_cm=0,last_location=p,
                     path=path,route_name=name,route_limit=90,route_done=False)
        yield wait_for(lambda:not state['route'],110)
        check(name+'_complete',state['route_done'])
        report.setdefault('crossings',[]).append({'name':name,'walked_m':state['walked_cm']/100,
                                                 'seconds':time.monotonic()-state['route_start']})
        capture(name+'-end')
        yield delay(2)
    state.pop('input',None)
    levels.editor_request_end_play()
    yield wait_for(lambda:not levels.is_in_play_in_editor())
    report['passed']=True


def complete(error=None):
    if error:report['error']=error;report['passed']=False
    if state['frames']:
        values=sorted(state['frames']);report['PIE_slate_frame_sample']={'count':len(values),'mean_ms':sum(values)/len(values)*1000,'p95_ms':values[int(len(values)*.95)]*1000,
        'caveat':'Editor slate tick includes editor overhead and screenshot frames; not standalone GPU benchmark'}
    (OUT/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    state.pop('input',None)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    unreal.unregister_slate_post_tick_callback(handle)


flow=run();pending=None


def tick(delta):
    global pending
    try:
        now=time.monotonic()
        if state.get('route'):
            pawn=state['pawn'];pc=state['pc'];p=pawn.get_actor_location()
            displacement=(p-state['last_location']).length();state['walked_cm']+=displacement;state['last_location']=p
            state['frames'].append(delta)
            path=state['path'];target=path[state['route_index']]
            dx,dy=target[0]*100-p.x,target[1]*100-p.y
            if math.hypot(dx,dy)<180:
                state['route_index']+=1
                if state['route_index']>=len(path):
                    state['route_done']=True;state['route']=False
                    if state['route_name']=='main':report['route_complete']=True
            if state['route']:
                yaw=math.degrees(math.atan2(dy,dx))
                pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=yaw,roll=0))
                state['input'].inject_input_vector_for_action(state['action'],unreal.Vector(0,1,0),[],[])
            if now-state['last_sample']>2:
                report['samples'].append({'route':state['route_name'],'t':round(now-state['route_start'],2),'position_cm':[p.x,p.y,p.z],
                                         'route_index':state['route_index'],'on_ground':pawn.character_movement.is_moving_on_ground(),
                                         'natural_loaded':len(unreal.GameplayStatics.get_all_actors_with_tag(state['world'],'TASK026.REBUILD'))})
                state['last_sample']=now
                (OUT/'progress.json').write_text(json.dumps({'walked_m':state['walked_cm']/100,'seconds':now-state['route_start'],'route_index':state['route_index'],'points':len(path)}),encoding='utf-8')
            if now-state['last_capture']>5:
                capture(f'{state["route_name"]}-{int(now-state["route_start"]):05}',640,360);state['last_capture']=now
            if now-state['progress_at']>15:
                check('route_no_stuck', (p-state['progress_location']).length()>200)
                check('route_no_fall_through',p.z>-2000)
                state['progress_at']=now;state['progress_location']=p
            if now-state['route_start']>state['route_limit']:state['route']=False
        if pending:
            predicate,deadline=pending
            if not predicate():
                if now>deadline:raise TimeoutError('PIE validation stage exceeded its deadline')
                return
        pending=next(flow)
    except StopIteration:complete()
    except Exception:complete(traceback.format_exc())


handle=unreal.register_slate_post_tick_callback(tick)
