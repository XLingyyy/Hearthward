"""Inspect the reproduced route obstruction with native traces and navigation queries."""
import json,time,traceback,math
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir());out=Path(unreal.Paths.project_saved_dir())/'Task049/route-diagnostic';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'method':'Diagnostic scene-location fixture only: visibility traces, mesh bounds and local navigation queries. This is not a route walk.','checks':{}};st={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=60):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+10)
def finish(error=None):
    if error:report['error']=error
    if 'pawn' in st:
        report['position']=str(st['pawn'].get_actor_location());report['health']=st['game'].health
        unreal.GameplayStatics.set_global_time_dilation(st['world'],1)
    report['ok']=not error and all(report['checks'].values())
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
def run():
    ew=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();ew.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(8)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),game=p.get_component_by_class(unreal.HearthwardGameplayComponent))
    check('continue',st['ui'].execute_action('continue'));st['ui'].open_page('hud');yield delay(3)
    p.get_movement_component().disable_movement();p.set_actor_location(unreal.Vector(15954,-103530,60000),False,True);yield delay(8)
    p.set_actor_location(unreal.Vector(15954,-103530,13766),False,True);p.get_movement_component().set_movement_mode(unreal.MovementMode.MOVE_WALKING);yield delay(3)
    pc.set_control_rotation(unreal.Rotator(pitch=-15,yaw=-90));yield delay(.5)
    unreal.SystemLibrary.execute_console_command(w,'Shot SHOWUI filename="'+(out/'obstruction.png').as_posix()+'" -nosuffix',pc);yield delay(.5)
    report['traces']=[]
    for dz in [-65,0,65]:
        start=p.get_actor_location()+unreal.Vector(0,0,dz);end=unreal.Vector(16000,-104200,start.z)
        hit=unreal.SystemLibrary.line_trace_single(w,start,end,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[p],unreal.DrawDebugTrace.NONE,True)
        row={'height':dz,'hit':str(hit)}
        if hit:
            parts=hit.to_tuple();row['parts']=[str(x) for x in parts]
            component=parts[10]
            if isinstance(component,unreal.StaticMeshComponent):
                row['mesh']=str(component.static_mesh)
                try:row['affects_navigation']=component.get_editor_property('can_ever_affect_navigation')
                except Exception as e:row['nav_property_error']=str(e)

        report['traces'].append(row)
    report['mesh_bounds']={}
    for name in ['Rock','Tree','Stump']:
        mesh=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/SM_'+name)
        bounds=mesh.get_bounds();report['mesh_bounds'][name]={'origin':[bounds.origin.x,bounds.origin.y,bounds.origin.z],'extent':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]}
    yield delay(25)
    report['local_paths']=[]
    for x,y in [(160,-1040),(175,-1030),(180,-1050),(160,-1060),(140,-1040),(140,-1060),(200,-1040)]:
        goal=unreal.NavigationSystemV1.project_point_to_navigation(w,unreal.Vector(x*100,y*100,13800),None,None,unreal.Vector(600,600,3000))
        path=unreal.NavigationSystemV1.find_path_to_location_synchronously(w,p.get_actor_location(),goal,p) if goal else None
        report['local_paths'].append({'xy':[x,y],'goal':str(goal),'partial':path.is_partial() if path else None,'points':[str(v) for v in path.path_points] if path else []})
    finish()
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if st.get('walking'):
            at=st['pawn'].get_actor_location();report['actual_walk_distance_m']+=(at-st['last_walk_position']).length()/100;st['last_walk_position']=at
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError('Walking route blocked at '+str(report.get('route_index')))
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
