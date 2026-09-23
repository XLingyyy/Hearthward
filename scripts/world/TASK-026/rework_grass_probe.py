"""Four-source grass isolation in the existing editor world; no package saves.

ShowFlag.InstancedStaticMeshes affects all ordinary HISM, including discrete
grass; native Landscape grass has separate InstancedGrass relevance in UE5.8.
This is a diagnostic visibility override, not a final scene setting.
"""
from pathlib import Path
import json
import time
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir())
config = json.loads((ROOT/'Saved/Task026/ReworkV2/grass-probe.json').read_text(encoding='utf-8'))
OUT = ROOT/config['output']
OUT.mkdir(parents=True, exist_ok=False)
is_standalone = config.get('mode') == 'Standalone'
if is_standalone:
    worlds = [w for w in unreal.ObjectIterator(unreal.World)
              if w.get_path_name().split('.')[0] == '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
              and unreal.GameplayStatics.get_player_controller(w, 0)]
    assert len(worlds) == 1
    world = worlds[0]
else:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == '/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds'
ASSET = '/Game/Hearthward/Assets/NaturalWorld/Rebuild'
mat = unreal.load_asset(ASSET+'/Materials/M_Landscape')
gt = unreal.load_asset(ASSET+'/Foliage/GT_Meadow')
report = {'status':'RUNNING', 'scope':'editor source isolation; transient show flags; no saves',
          'engine':unreal.SystemLibrary.get_engine_version(), 'groups':[],
          'material':mat.get_path_name(), 'grass_type':gt.get_path_name()}
report['varieties'] = []
sample_grass = unreal.load_asset(ASSET+'/Foliage/GT_S1_Meadow')
report['sample_varieties'] = [{'mesh':v.grass_mesh.get_path_name(), 'density':v.grass_density.default,
    'cull_start':v.start_cull_distance.default, 'cull_end':v.end_cull_distance.default}
    for v in sample_grass.grass_varieties] if sample_grass else []
for v in gt.grass_varieties:
    report['varieties'].append({'mesh':v.grass_mesh.get_path_name(), 'density':v.grass_density.default,
        'cull_start':v.start_cull_distance.default, 'cull_end':v.end_cull_distance.default,
        'scale':[v.scale_z.min,v.scale_z.max]})
report['nodes'] = []
for n in ([] if is_standalone else unreal.ObjectIterator(unreal.MaterialExpression)):
    if n.get_outer() != mat:
        continue
    entry = {'name':n.get_name(), 'class':n.get_class().get_name()}
    if isinstance(n, unreal.MaterialExpressionTextureSample):
        entry['texture'] = n.texture.get_path_name() if n.texture else None
    if isinstance(n, unreal.MaterialExpressionLandscapeGrassOutput):
        entry['outputs'] = [{'name':str(g.get_editor_property('name')),
                             'type':g.get_editor_property('grass_type').get_path_name()}
                            for g in n.get_editor_property('grass_types')]
    report['nodes'].append(entry)

hit = unreal.SystemLibrary.line_trace_single(world,unreal.Vector(-111000,-69500,150000),
    unreal.Vector(-111000,-69500,-100000),unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    True,[],unreal.DrawDebugTrace.NONE,True).to_tuple()
assert hit[0]
position = unreal.Vector(-111000,-69500,hit[4].z+180)
rotation = unreal.Rotator(pitch=-16,yaw=0,roll=0)
if not is_standalone:
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(position,rotation)
report['camera'] = {'cm':[position.x,position.y,position.z],'rotation':[-16,0,0], 'ground_trace_cm':hit[4].z}
groups = [('G0',0,0),('G1',1,0),('G2',0,1),('G3',1,1)]
if 'groups' in config:
    requested = config['groups']
    assert requested and len(set(requested)) == len(requested)
    assert set(requested) <= {g[0] for g in groups}
    groups = [g for g in groups if g[0] in requested]
state = {'index':0,'phase':'setup','until':0}
levels = None if is_standalone else unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
is_pie = config.get('mode','editor') == 'PIE'
if is_standalone:
    state['pawn']=unreal.GameplayStatics.get_player_pawn(world,0)
    state['pc']=unreal.GameplayStatics.get_player_controller(world,0)
    state['pawn'].set_actor_location(position,False,True)
    state['pc'].set_control_rotation(rotation)
    state['until']=time.monotonic()+8
    report['scope']='Standalone source isolation and leave/return; diagnostic teleports; no movement/performance acceptance'
    report['hism_caveat']='ShowFlag.InstancedStaticMeshes also hides HISM trees and rocks; native grass uses a separate flag'
if is_pie:
    state['phase']='begin_pie'
    report['scope']='PIE source isolation; diagnostic teleports only; no route acceptance'
unreal.EditorPythonScripting.set_keep_python_script_alive(True)


def console(command):
    unreal.SystemLibrary.execute_console_command(world,command)


def finish(error=None):
    console('ShowFlag.InstancedStaticMeshes 2')
    console('grass.Enable 1')
    unreal.unregister_slate_post_tick_callback(handle)
    if is_pie and levels.is_in_play_in_editor():levels.editor_request_end_play()
    report['status'] = 'FAIL' if error else 'CAPTURED'
    if error:report['error']=error
    (OUT/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)


def tick(dt):
    global world
    try:
        if time.monotonic()<state['until']:return
        if state['phase']=='begin_pie':
            levels.editor_request_begin_play()
            state.update(phase='wait_pie',until=time.monotonic()+8)
            return
        if state['phase']=='wait_pie':
            world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            if not world:return
            state['pawn']=unreal.GameplayStatics.get_player_pawn(world,0)
            state['pc']=unreal.GameplayStatics.get_player_controller(world,0)
            if not state['pawn']:return
            state['pawn'].set_actor_location(position,False,True)
            state['pc'].set_control_rotation(rotation)
            state.update(phase='setup',until=time.monotonic()+8)
            return
        if state['index']==len(groups):
            finish(); return
        name,hism,grass = groups[state['index']]
        if state['phase']=='setup':
            console('ShowFlag.InstancedStaticMeshes '+str(hism))
            console('grass.Enable '+str(grass))
            state.update(phase='capture',until=time.monotonic()+12)
        elif state['phase'] in ['capture','return_capture']:
            returning=state['phase']=='return_capture'
            components=[]
            for c in unreal.ObjectIterator(unreal.GrassInstancedStaticMeshComponent):
                owner=c.get_owner()
                if owner and owner.get_world()==world:
                    p=c.get_world_location()
                    components.append({'owner':owner.get_path_name(),'world':world.get_path_name(),
                        'position_cm':[p.x,p.y,p.z],'instances':c.get_instance_count()})
            report['groups'].append({'name':name,'visit':'return' if returning else 'initial',
                'hism_visible':hism,'landscape_grass_enabled':grass,
                'native_components':components,'grass_enable':unreal.SystemLibrary.get_console_variable_int_value('grass.Enable'),
                'grass_density_scale':unreal.SystemLibrary.get_console_variable_float_value('grass.densityScale')})
            if not is_pie:console('grass.DumpGrassData')
            state['phase']='busy'
            state['returning']=returning
            filename=OUT/(name+('-return' if returning else '')+'.png')
            if is_standalone:
                console('HighResShot 1920x1080 filename="'+str(filename).replace('\\','/')+'"')
            else:
                state['capture']=unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,
                    str(filename),force_game_view=True)
            state.update(phase='wait',until=time.monotonic()+2)
        elif state['phase']=='wait' and (is_standalone or state['capture'].is_task_done()):
            if state['returning']:
                state.update(index=state['index']+1,phase='setup')
            else:
                far=unreal.Vector(-100000,50000,150000)
                trace=unreal.SystemLibrary.line_trace_single(world,far,far-unreal.Vector(0,0,250000),
                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE,True).to_tuple()
                assert trace[0]
                far.z=trace[4].z+180
                if is_pie or is_standalone:state['pawn'].set_actor_location(far,False,True)
                else:unreal.EditorLevelLibrary.set_level_viewport_camera_info(far,rotation)
                state.update(phase='away',until=time.monotonic()+15)
        elif state['phase']=='away':
            if is_pie or is_standalone:
                state['pawn'].set_actor_location(position,False,True)
                state['pc'].set_control_rotation(rotation)
            else:unreal.EditorLevelLibrary.set_level_viewport_camera_info(position,rotation)
            state.update(phase='return_capture',until=time.monotonic()+15)
    except Exception:
        finish(traceback.format_exc())


handle=unreal.register_slate_post_tick_callback(tick)
