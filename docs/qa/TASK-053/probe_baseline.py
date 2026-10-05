"""Diagnostic only: existing rendered PIE, actual brother and configured natural water. No asset/save changes."""
from pathlib import Path
import unreal,json,time,traceback
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir());out=root/'Saved/Task053/baseline';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={'ok':False,'method':__doc__,'observations':{}}
def delay(seconds):
 until=time.monotonic()+seconds
 return lambda:time.monotonic()>=until
def run():
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0);pawn=unreal.GameplayStatics.get_player_pawn(world,0)
 unreal.GameplayStatics.set_game_paused(world,False)
 pawn.get_component_by_class(unreal.HearthwardGameplayComponent).set_editor_property('enabled',True)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc);yield delay(.5)
 brother=unreal.GameplayStatics.get_actor_of_class(world,unreal.HearthwardCompanionFixture)
 movement=brother.get_component_by_class(unreal.CharacterMovementComponent)
 report['observations']['brother_traversal_missing']=brother.get_component_by_class(unreal.HearthwardTraversalComponent) is None
 report['observations']['brother_movement_class']=movement.get_class().get_name()
 row=json.loads((root/'Resources/Data/experience.json').read_text(encoding='utf-8'))['water'][0]
 point=unreal.Vector(row['center_m'][0]*100,row['center_m'][1]*100,row['surface_m']*100-20)
 brother.set_actor_location(point,False,True);movement.stop_movement_immediately()
 yield delay(2)
 report['observations']['natural_water_mode']=str(movement.get_editor_property('movement_mode'))
 report['observations']['natural_water_position']=str(brother.get_actor_location())
 report['reproduced_missing_shared_traversal']=report['observations']['brother_traversal_missing'] and report['observations']['brother_movement_class']=='CharacterMovementComponent'
 report['ok']=True
runner=run();pending=None;deadline=time.monotonic()+150
def tick(delta):
 global pending
 try:
  if time.monotonic()>deadline:raise TimeoutError('baseline diagnostic')
  if pending and not pending():return
  pending=next(runner)
 except StopIteration:finish()
 except Exception:
  report['error']=traceback.format_exc();finish()
def finish():
 levels.editor_request_end_play();(out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
