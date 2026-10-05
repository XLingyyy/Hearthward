"""Diagnostic structured navigation: 150m natural route return; setup positioning and suspended player tactical tick isolate the real Navigation executor and collision, no normal UI/model acceptance claim."""
from pathlib import Path
import unreal,json,time,traceback,math
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir());out=root/'Saved/Task053/long-return';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={'ok':False,'method':__doc__,'samples':[]}
state={}
def delay(seconds):
 until=time.monotonic()+seconds
 return lambda:time.monotonic()>=until
def run():
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0);pawn=unreal.GameplayStatics.get_player_pawn(world,0)
 unreal.GameplayStatics.set_game_paused(world,False)
 game=pawn.get_component_by_class(unreal.HearthwardGameplayComponent);game.set_editor_property('enabled',True)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc);yield delay(.5)
 brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
 game.order_companion('wait')
 game.set_component_tick_enabled(False)
 navigation=brother.get_component_by_class(unreal.HearthwardCompanionNavigationComponent)
 route=json.loads((root/'docs/world/TASK-049/terrain-route.json').read_text(encoding='utf-8'))['path']
 origin=pawn.get_actor_location()
 row=next(p for p in route if math.hypot(p[0]*100-origin.x,p[1]*100-origin.y)>=15000)
 point=unreal.Vector(row[0]*100,row[1]*100,row[2]*100+82)
 brother.set_actor_location(point,False,True)
 yield delay(5)
 movement=brother.get_component_by_class(unreal.CharacterMovementComponent)
 survival=brother.get_component_by_class(unreal.HearthwardSurvivalComponent)
 report['diagnostics']={'brother_controller':str(brother.get_controller()),'brother_life':str(survival.state.life),'movement_mode':str(movement.get_editor_property('movement_mode')),'player_position':str(origin)}
 projected=unreal.NavigationSystemV1.project_point_to_navigation(world,brother.get_actor_location(),None,None,unreal.Vector(80,80,200))
 target_projected=unreal.NavigationSystemV1.project_point_to_navigation(world,origin,None,None,unreal.Vector(80,80,200))
 path=unreal.NavigationSystemV1.find_path_to_location_synchronously(world,brother.get_actor_location(),origin,brother)
 report['diagnostics'].update(local_projection=str(projected),target_projection=str(target_projected),path_points=len(path.path_points) if path else 0,path_partial=path.is_partial() if path else None)
 state.update(navigation=navigation,brother=brother,target=pawn)
 report['start_distance_m']=(brother.get_actor_location()-origin).length()/100
 report['start_position']=str(brother.get_actor_location());state['active']=True
 yield delay(35)
 state['active']=False;navigation.stop()
 report['end_position']=str(brother.get_actor_location())
 report['end_distance_m']=(brother.get_actor_location()-origin).length()/100
 report['arrived']=navigation.is_at(pawn,100)
 report['reproduced_stalled_return']=report['start_distance_m']-report['end_distance_m']<2 and not report['arrived']
 report['ok']=report['arrived']
runner=run();pending=None;deadline=time.monotonic()+190
def tick(delta):
 global pending
 try:
  if time.monotonic()>deadline:raise TimeoutError('long return diagnostic')
  if state.get('active'):
   accepted=state['navigation'].move_to_actor(state['target'],600,100)
   if not report['samples'] or time.monotonic()-report['samples'][-1]['wall']>=1:
    report['samples'].append({'wall':time.monotonic(),'accepted':accepted,'position':str(state['brother'].get_actor_location()),'status':state['navigation'].get_status()})
  if pending and not pending():return
  pending=next(runner)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
def finish():
 levels.editor_request_end_play();(out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
