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
SRC=ROOT/'art_source/TASK-026/Rebuild'
MAP='/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
config_path=SRC/'verification_options.json'
options=json.loads(config_path.read_text()) if config_path.exists() else {'route_seconds':90}
runtime=options.get('runtime','PIE')
if runtime not in ['PIE','Standalone']:
    raise ValueError('Runtime must be PIE or Standalone')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem) if runtime=='PIE' else None
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem) if runtime=='PIE' else None
mode=options.get('mode','smoke')
if mode not in ['smoke','route_full','sample_loop']:
    raise ValueError('Supported modes: smoke, route_full, sample_loop')
OUT=ROOT/options.get('output','Saved/Task026/ReworkV2/walk-'+time.strftime('%Y%m%d-%H%M%S'))
OUT.mkdir(parents=True,exist_ok=False)
routes=json.loads((SRC/'routes.json').read_text())
if mode=='sample_loop':
    routes=json.loads((SRC/'ReworkV2/s1-route.json').read_text(encoding='utf-8'))
    if options.get('direction','forward')=='reverse':
        routes['loop']=list(reversed(routes['loop']))
    elif options.get('direction','forward')!='forward':
        raise ValueError('Sample direction must be forward or reverse')
report={'passed':False,'map':MAP,'checks':{},'samples':[],'captures':[],
        'mode':mode,'acceptance_scope':'local movement only' if mode=='smoke' else 'main loop walking only; branches and crossings reported separately',
        'route_complete':False,'input_method':'Existing Enhanced Input, default 350 cm/s, no route teleport',
        'scope':runtime+'; sample movement and initial cost only, not final R6 performance acceptance'}
state={'forward':False,'route':False,'frames':[]}
if mode=='sample_loop':
    report['acceptance_scope']='S1 sample loop only; all checkpoints required'
    report['direction']=options.get('direction','forward')
    report['planned_length_m']=routes['length_m']
unreal.EditorPythonScripting.set_keep_python_script_alive(True)


def check(key,value):
    report['checks'][key]=bool(value)
    if not value:raise AssertionError(key)


def wait_for(predicate,seconds=60):return predicate,time.monotonic()+seconds


def delay(seconds):
    until=time.monotonic()+seconds
    return wait_for(lambda:time.monotonic()>until,seconds+30)


def capture(name,width=1280,height=720):
    if runtime=='Standalone':width,height=1920,1080
    p=OUT/(name+'.png')
    unreal.SystemLibrary.execute_console_command(state['world'],f'HighResShot {width}x{height} filename="{p.as_posix()}"',state['pc'])
    report['captures'].append(name+'.png')


def run():
    if runtime=='PIE':
        check('map_reopens',levels.load_level(MAP))
        levels.editor_request_begin_play()
        yield wait_for(levels.is_in_play_in_editor)
        state['world']=editor.get_game_world()
    else:
        worlds=[w for w in unreal.ObjectIterator(unreal.World)
                if w.get_path_name().split('.')[0]==MAP and unreal.GameplayStatics.get_player_controller(w,0)]
        check('single_standalone_world',len(worlds)==1)
        state['world']=worlds[0]
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
    if runtime=='Standalone':
        report['csv_file']='TASK026-'+OUT.name+'.csv'
        unreal.SystemLibrary.execute_console_command(world,'CsvProfile STARTFILE='+report['csv_file'],pc)
        unreal.SystemLibrary.execute_console_command(world,'CsvProfile START',pc)
        state['csv_active']=True
    state.update(route=True,route_index=1,route_start=time.monotonic(),last_sample=0,last_capture=0,
                 progress_at=time.monotonic(),progress_location=p,walked_cm=0,last_location=p,
                 path=routes['loop'],route_name='main',route_limit=options.get('route_seconds',90),route_done=False)
    yield wait_for(lambda: not state['route'],options.get('route_seconds',90)+90)
    check('walked_at_least_100m',state['walked_cm']>=10000)
    if mode!='smoke':check('all_main_loop_checkpoints_reached',state['route_done'])
    report['route_walked_m']=state['walked_cm']/100
    report['route_seconds']=time.monotonic()-state['route_start']
    if state.get('csv_active'):
        unreal.SystemLibrary.execute_console_command(world,'CsvProfile STOP',pc)
        state['csv_active']=False
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
    if runtime=='PIE':
        levels.editor_request_end_play()
        yield wait_for(lambda:not levels.is_in_play_in_editor())
    report['passed']=True


def complete(error=None):
    if error:report['error']=error;report['passed']=False
    if state['frames']:
        values=sorted(state['frames']);report[runtime+'_slate_frame_sample']={'count':len(values),'mean_ms':sum(values)/len(values)*1000,'p95_ms':values[int(len(values)*.95)]*1000,
        'caveat':'Diagnostic slate tick; use native CSV for CPU/GPU performance figures'}
    (OUT/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    state.pop('input',None)
    if state.get('csv_active'):
        unreal.SystemLibrary.execute_console_command(state['world'],'CsvProfile STOP',state['pc'])
        state['csv_active']=False
    if runtime=='PIE' and levels.is_in_play_in_editor():levels.editor_request_end_play()
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
            if now-state['progress_at']>15:
                check('route_no_stuck', (p-state['progress_location']).length()>200)
                half=pawn.capsule_component.get_scaled_capsule_half_height()
                ground=unreal.SystemLibrary.line_trace_single(state['world'],p,
                    p-unreal.Vector(0,0,half+300),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                    True,[pawn],unreal.DrawDebugTrace.NONE,True).to_tuple()
                check('route_ground_below_capsule',ground[0] and -10 <= p.z-half-ground[4].z <= 300)
                state['progress_at']=now;state['progress_location']=p
            if now-state['route_start']>state['route_limit']:
                state['route']=False
                if mode!='smoke' or state['route_name']!='main':
                    raise TimeoutError('Route deadline reached before all checkpoints')
        if pending:
            predicate,deadline=pending
            if not predicate():
                if now>deadline:raise TimeoutError('PIE validation stage exceeded its deadline')
                return
        pending=next(flow)
    except StopIteration:complete()
    except Exception:complete(traceback.format_exc())


handle=unreal.register_slate_post_tick_callback(tick)
