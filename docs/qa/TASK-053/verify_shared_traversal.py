"""Rendered PIE regression: real brother/natural lake. Isolated setup positions; no asset/save changes."""
from pathlib import Path
import unreal,json,time,traceback
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir());out=root/'Saved/Task053/shared-traversal';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={'ok':False,'method':__doc__,'observations':{},'checks':{}}
state={}
def delay(seconds):
 until=time.monotonic()+seconds
 return lambda:time.monotonic()>=until
def run():
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0);pawn=unreal.GameplayStatics.get_player_pawn(world,0)
 unreal.GameplayStatics.set_game_paused(world,False)
 game=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
 game.set_editor_property('enabled',True)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc);yield delay(.5)
 brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
 movement=brother.get_component_by_class(unreal.CharacterMovementComponent)
 traversal=brother.get_component_by_class(unreal.HearthwardTraversalComponent)
 survival=brother.get_component_by_class(unreal.HearthwardSurvivalComponent)
 report['checks']['brother_shared_traversal']=traversal is not None
 report['checks']['shared_swimming_physics']=movement.get_class().get_name()=='HearthwardMovementComponent'
 row=json.loads((root/'Resources/Data/experience.json').read_text(encoding='utf-8'))['water'][0]
 surface=row['surface_m']*100
 point=unreal.Vector(row['center_m'][0]*100,row['center_m'][1]*100,surface-20)
 brother.set_actor_location(point,False,True);movement.stop_movement_immediately()
 yield delay(2)
 report['observations']['natural_water_mode']=str(movement.get_editor_property('movement_mode'))
 report['observations']['natural_water_position']=str(brother.get_actor_location())
 report['checks']['natural_lake_swimming']=movement.get_editor_property('movement_mode')==unreal.MovementMode.MOVE_SWIMMING
 report['checks']['floats_near_surface']=abs(brother.get_actor_location().z-(surface-20))<80
 game.order_companion('wait');game.set_component_tick_enabled(False)
 state['paddling']=brother
 started=unreal.GameplayStatics.get_time_seconds(world)
 yield lambda:unreal.GameplayStatics.get_time_seconds(world)-started>=26
 state['paddling']=None
 report['observations']['paddle_active_seconds']=unreal.GameplayStatics.get_time_seconds(world)-started
 before=brother.get_actor_location().z;yield delay(2)
 report['observations']['exhausted_sink_cm']=before-brother.get_actor_location().z
 report['checks']['exhaustion_sinks_physically']=60<report['observations']['exhausted_sink_cm']<160
 report['ok']=all(report['checks'].values())
runner=run();pending=None;deadline=time.monotonic()+150
def tick(delta):
 global pending
 try:
  if time.monotonic()>deadline:raise TimeoutError('shared traversal diagnostic')
  if state.get('paddling'):state['paddling'].add_movement_input(unreal.Vector(1,0,0),1,False)
  if pending and not pending():return
  pending=next(runner)
 except StopIteration:finish()
 except Exception:
  report['error']=traceback.format_exc();finish()
def finish():
 levels.editor_request_end_play();(out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
