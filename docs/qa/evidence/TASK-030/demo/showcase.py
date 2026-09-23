import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/'Saved/DemoValidation'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def world():return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def wait(pred,secs=60):return pred,time.monotonic()+secs
def delay(secs):
 end=time.monotonic()+secs
 return wait(lambda:time.monotonic()>=end,secs+10)
report={'views':[]}
def run():
 levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
 unreal.GameplayStatics.get_player_controller(world(),0).get_hud().screen.execute_action('continue')
 yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_HearthwardWilds');yield delay(5)
 p=unreal.GameplayStatics.get_player_pawn(world(),0);pc=unreal.GameplayStatics.get_player_controller(world(),0)
 b=p.get_component_by_class(unreal.HearthwardBuildingComponent);ui=pc.get_hud().screen
 c=unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.HearthwardCompanionFixture)[0]
 buildings=b.get_buildings()
 table=next(a for a in buildings if not a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent))
 bed=next(a for a in buildings if (t:=a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent)) and str(t.kind)=='bed')
 fire=next(a for a in buildings if (t:=a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent)) and str(t.kind)=='campfire')
 for name,actor in [('workbench',table),('bed',bed),('storage',c.camp),('campfire',fire)]:
  loc=actor.get_actor_location()
  p.set_actor_location(loc+unreal.Vector(175,175,110),False,True);p.character_movement.stop_movement_immediately()
  pc.set_control_rotation(unreal.Rotator(pitch=-25,yaw=-135))
  yield delay(2)
  report['views'].append({'name':name,'location':str(loc),'time':time.monotonic()})
  (out/'showcase-progress.json').write_text(json.dumps(report),encoding='utf-8')
  if name=='workbench':ui.open_page('crafting');yield delay(3);ui.open_page('hud')
  elif name=='campfire':
   p.set_actor_location(loc+unreal.Vector(140,0,100),False,True);yield delay(1)
   p.get_component_by_class(unreal.HearthwardInteractionComponent).interact_nearest()
  yield delay(6)
 report['ok']=True
 (out/'showcase-result.json').write_text(json.dumps(report),encoding='utf-8')
 yield delay(30)
flow=run();pending=None
def tick(_dt):
 global pending
 try:
  if pending:
   pred,end=pending
   if not pred():
    if time.monotonic()>end:raise TimeoutError('showcase wait')
    return
  pending=next(flow)
 except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
 except Exception:
  report['error']=traceback.format_exc();(out/'showcase-result.json').write_text(json.dumps(report),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
