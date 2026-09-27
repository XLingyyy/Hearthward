"""Walk the dry route in the real natural map; time dilation speeds QA, not movement speed."""
import json,time,traceback,math
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir());out=Path(unreal.Paths.project_saved_dir())/'Task049/route';out.mkdir(parents=True,exist_ok=True)
route=json.loads((root/'docs/world/TASK-049/terrain-route.json').read_text(encoding='utf-8'))['path']
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
resume_file=root/'.agent-local/route-resume.json'
resume=json.loads(resume_file.read_text(encoding='utf-8')) if resume_file.exists() else None
report={'ok':False,'method':'Real map, capsule movement input along local navigation paths, no position teleport during route. Global time dilation 3 for QA. Source dry-ground checks separate.','checks':{}};st={}
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
def move(goal):
    here=st['pawn'].get_actor_location();key=(goal.x,goal.y)
    if st.get('move_goal')!=key:
        st.update(move_goal=key,last_motion=here,motion_at=time.monotonic(),detour=None,detour_attempt=0)
    delta=unreal.Vector(goal.x-here.x,goal.y-here.y,0)
    if delta.length()<70:st['detour']=None;return True
    if (here-st['last_motion']).length()>15:
        st['last_motion']=here;st['motion_at']=time.monotonic()
    elif time.monotonic()-st['motion_at']>2 and st['detour_attempt']<4:
        # Small walking sidesteps around a blocking rock/tree; collision stays enabled.
        side=1 if st['detour_attempt']%2==0 else -1
        sideways=unreal.Vector(-delta.y,delta.x,0)/delta.length()
        st['detour']=here+sideways*(400*side)
        st['detour_attempt']+=1;st['motion_at']=time.monotonic()
        report.setdefault('walking_detours',[]).append({'index':report['route_index'],'from':str(here),'to':str(st['detour'])})
    if st['detour'] is not None:
        step=st['detour']-here;step.z=0
        if step.length()<70:st['detour']=None;st['motion_at']=time.monotonic()
        else:delta=step
    st['pc'].set_control_rotation(unreal.Rotator(pitch=-8,yaw=unreal.MathLibrary.deg_atan2(delta.y,delta.x)))
    st['pawn'].add_movement_input(delta/delta.length(),min(1,delta.length()/120),False)
    return False

def run():
    ew=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();ew.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(8)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property('screen'),game=p.get_component_by_class(unreal.HearthwardGameplayComponent),bag=p.get_component_by_class(unreal.HearthwardInventoryComponent))
    c=next(x for x in unreal.ObjectIterator(unreal.HearthwardCampaignSubsystem) if x.get_outer()==w)
    check('continue campaign',st['ui'].execute_action('continue'));st['ui'].open_page('hud');yield delay(5)
    c.interact();check('ordinary travel back to camp',c.travel('camp'));yield wait(lambda:not c.busy(),120);yield delay(3)
    check('at route start',math.hypot(p.get_actor_location().x+98000,p.get_actor_location().y+75000)<1000)
    first=0
    if resume:
        first=int(resume['index']);report['segment_start_fixture']=resume
        p.get_movement_component().disable_movement();p.set_actor_location(unreal.Vector(resume['x'],resume['y'],60000),False,True);yield delay(6)
        p.set_actor_location(unreal.Vector(resume['x'],resume['y'],resume['z']+20),False,True);p.get_movement_component().set_movement_mode(unreal.MovementMode.MOVE_WALKING);yield delay(2)
    st['bag'].try_add('roast',8)
    unreal.GameplayStatics.set_global_time_dilation(w,3);started=unreal.GameplayStatics.get_time_seconds(w)
    report['path_points']=len(route);report['walk_distance_m']=0;report['actual_walk_distance_m']=0;previous=p.get_actor_location();st['last_walk_position']=previous;st['walking']=True
    for index in range(first,len(route)):
        x,y,z=route[index]
        report['route_index']=index
        goal=unreal.Vector(x*100,y*100,z*100+90)
        # Terrain samples do not account for the footprint of rocks and foliage.
        # Project each walking target onto nearby navigation; never move the pawn by projection.
        def local_path():
            projected=unreal.NavigationSystemV1.project_point_to_navigation(w,goal,None,None,unreal.Vector(600,600,600))
            path=unreal.NavigationSystemV1.find_path_to_location_synchronously(w,p.get_actor_location(),projected,p) if projected else None
            report['navigation_goal']=str(projected);report['navigation_partial']=path.is_partial() if path else None
            if path and len(path.path_points)>0 and not path.is_partial():st['path']=path;return True
            return False
        if local_path():
            for point in st['path'].path_points[1:]:yield wait(lambda point=point:move(point),25)
        else:
            report.setdefault('navigation_gaps',[]).append(index)
            # A player's physical walk is independent of Recast path availability.
            # Keep collision, gravity and movement limits enabled across this sampled segment.
            yield wait(lambda:move(goal),25)
        here=p.get_actor_location();report['walk_distance_m']+=(here-previous).length()/100;previous=here
        if st['game'].hunger<65:st['game'].use_item('roast')
        check('alive on foot',st['game'].health>0 and p.get_movement_component().movement_mode==unreal.MovementMode.MOVE_WALKING)
        report['position']=[here.x,here.y,here.z];report['game_seconds']=unreal.GameplayStatics.get_time_seconds(w)-started
        if index%10==0:(out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    check('route reaches dry hometown entry',math.hypot(p.get_actor_location().x-101000,p.get_actor_location().y-25000)<1200)
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
