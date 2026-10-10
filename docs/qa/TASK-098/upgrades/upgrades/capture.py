import unreal,json,time,traceback
from pathlib import Path
out=Path(__file__).parent/'visual';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'PROTOTYPE_ONLY static assembly review in rendered PIE, not paid upgrade or full playthrough.','views':[]}
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
pending=None;busy=False
def delay(s):
 end=time.monotonic()+s
 return lambda:time.monotonic()>=end
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 mat=unreal.load_asset('/Game/Hearthward/Assets/TASK-098/Upgrades/M_UpgradeWood')
 unreal.MaterialEditingLibrary.set_material_instance_parent(mat,unreal.load_asset('/Game/Hearthward/Assets/TASK-096/Nearfield/M_OldTimber'))
 unreal.MaterialEditingLibrary.update_material_instance(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
 ed=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 for a in ed.get_all_level_actors():
  if isinstance(a,unreal.DirectionalLight):ed.destroy_actor(a)
 sun=ed.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1000),unreal.Rotator(pitch=-40,yaw=-40));sun.light_component.set_intensity(3)
 floor=ed.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-5));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(100,100,.1))
 bases={'Workbench':'Workbench/SM_Workbench_Practical','Smelter':'CampSet/SM_Smelter','Forge':'Forge/SM_Forge_Practical','Cooking':'CampSet/SM_Cooking'}
 for i,(kind,base) in enumerate(bases.items()):
  for level in [1,2,3]:
   origin=unreal.Vector(i*500,level*400,0)
   paths=[base]+['Upgrades/SM_'+kind+'_L'+str(n) for n in range(2,level+1)]+(['Upgrades/SM_ForgeHearth'] if kind=='Forge' else [])
   for path in paths:
    a=ed.spawn_actor_from_class(unreal.StaticMeshActor,origin);a.static_mesh_component.set_static_mesh(unreal.load_asset('/Game/Hearthward/Assets/TASK-098/'+path))
   c=ed.spawn_actor_from_class(unreal.CameraActor,origin+unreal.Vector(200,-260,160));c.tags=[kind+str(level)]
   c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(c.get_actor_location(),origin+unreal.Vector(0,0,58)),False)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(4)
 w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);unreal.GameplayStatics.set_game_paused(w,False)
 for kind in bases:
  for level in [1,2,3]:
   tag=kind+str(level);c=unreal.GameplayStatics.get_all_actors_with_tag(w,tag)[0];pc.set_view_target_with_blend(c,0);yield delay(2)
   assert (pc.player_camera_manager.get_camera_location()-c.get_actor_location()).length()<5
   unreal.SystemLibrary.execute_console_command(w,'Shot filename='+str(out/(tag+'.png'))+' -nosuffix');yield delay(1)
   report['views'].append(tag)
 report['passed']=True
flow=run();deadline=time.monotonic()+240
def finish():
 (out/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('upgrade assembly review')
  if pending and not pending():return
  pending=next(flow)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
