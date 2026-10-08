import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-096/nearfield';out.mkdir(parents=True,exist_ok=True)
frames=out/'frames';frames.mkdir(exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);lib=unreal.MaterialEditingLibrary
report={'passed':False,'method':'Actual natural map new-game UI action in isolated save pool; capture camera only. Three actual game viewport camera views; production lighting unchanged.','samples':[]}
pending=None;busy=False
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def subsystem(cls,world):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 for label in ['RoughStone','OldTimber']:
  mat=unreal.load_asset('/Game/Hearthward/Assets/TASK-096/Nearfield/M_'+label)
  nodes=lib.get_material_expressions(mat)
  normal=next(n for n in nodes if isinstance(n,unreal.MaterialExpressionMaterialFunctionCall) and n.get_editor_property('material_function').get_name()=='WorldAlignedNormal')
  static=next((n for n in nodes if isinstance(n,unreal.MaterialExpressionStaticBool)),None)
  if static is None:static=lib.create_material_expression(mat,unreal.MaterialExpressionStaticBool)
  static.set_editor_property('value',True);assert lib.connect_material_expressions(static,'',normal,'WorldSpace')
  for node in nodes:
   if isinstance(node,unreal.MaterialExpressionTextureObject) and node.get_editor_property('texture').get_name().endswith('_N'):node.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
  lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
 levels.load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
 editor_world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
 editor_world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
 editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 capture=editor.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,25000));capture.tags=['Task096Capture'];capture.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
 ui=pc.get_hud().get_editor_property('screen');report['new_action']=ui.execute_action('new');yield delay(8)
 unreal.GameplayStatics.set_game_paused(world,False);yield delay(2)
 homes=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardHometownFortress'));assert len(homes)==1,len(homes);home=homes[0]
 parts=home.get_components_by_class(unreal.StaticMeshComponent);frame=next(c for c in parts if c.get_name()=='BedroomDoorframe')
 assert frame.static_mesh.get_name()=='SM_BedroomDoorframe';assert frame.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
 clock=subsystem(unreal.HearthwardWorldClockSubsystem,world);report['minute_of_day']=clock.get_snapshot().minute_of_day
 report['frame']={'mesh':frame.static_mesh.get_path_name(),'location':str(frame.relative_location),'scale':str(frame.relative_scale3d),'collision':str(frame.get_collision_enabled())}
 report['materials']={label:[t.get_path_name() for t in lib.get_material_used_textures(unreal.load_asset('/Game/Hearthward/Assets/TASK-096/Nearfield/M_'+label))] for label in ['RoughStone','OldTimber']}
 for refs in report['materials'].values():assert len(refs)==2,refs
 origin=home.get_actor_location();floor=frame.relative_location.z
 camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor) if a.actor_has_tag('Task096Capture'))
 camera.camera_component.set_field_of_view(78)
 pc.set_view_target_with_blend(camera,0)
 views=[('bedroom',unreal.Vector(-230,70,floor+180),unreal.Vector(0,550,floor+210)),('gallery',unreal.Vector(720,830,floor+190),unreal.Vector(0,550,floor+190)),('beam',unreal.Vector(300,-200,floor+180),unreal.Vector(0,0,floor+400))]
 for name,pos,look in views:
  camera.set_actor_location(origin+pos,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(origin+pos,origin+look),False)
  yield delay(3)
  unreal.SystemLibrary.execute_console_command(world,'Shot filename='+str(frames/('viewport-night-'+name+'.png'))+' -nosuffix');yield delay(1)
  report['samples'].append('viewport-night-'+name)
 report['passed']=True
iterator=run();deadline=time.monotonic()+240
def finish():
 (out/'preview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('Nearfield capture')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
