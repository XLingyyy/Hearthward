import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-095/costumes';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'fixture':'PROTOTYPE_ONLY unsaved cast review; production meshes and materials','roles':[]}
pending=None
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
 actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(800,0,300));capture.tags=['Task095CostumeCapture']
 capture.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(600,0,450));light.tags=['Task095CostumeLight']
 light.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(1)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
 unreal.GameplayStatics.set_game_paused(world,False)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.CastPreview')
 capture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('Task095CostumeCapture'))
 light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('Task095CostumeLight'))
 light.point_light_component.set_intensity(700);light.point_light_component.set_attenuation_radius(2500)
 for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Character):
  role=next((str(tag).split('.')[-1] for tag in actor.tags if str(tag).startswith('Task095Cast.')),None)
  if not role:continue
  materials=[m.get_path_name() if m else None for m in actor.mesh.get_materials()]
  report['roles'].append({'role':role,'mesh':actor.mesh.skeletal_mesh_asset.get_path_name(),'materials':materials,
                          'scale':str(actor.mesh.get_editor_property('RelativeScale3D'))})
  if role in ('Hero','Brother','Civilian'):
   assert materials[0].endswith('M_'+role+'_CoarseCloth.M_'+role+'_CoarseCloth'),materials
   mesh_role='Hero' if role=='Hero' else 'Brother'
   assert actor.mesh.skeletal_mesh_asset.get_path_name()==f'/Game/Characters/{mesh_role}/UE5/SK_{mesh_role}.SK_{mesh_role}'
 assert len(report['roles'])==6,report['roles']
 target=unreal.RenderingLibrary.create_render_target2d(world,1600,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 c=capture.capture_component2d;c.texture_target=target;c.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.fov_angle=50
 yield delay(5)
 for name,look,offset in [('cast',unreal.Vector(0,0,90),unreal.Vector(1600,0,300)),
                           ('brothers-civilian',unreal.Vector(0,-300,90),unreal.Vector(900,0,130)),
                           ('enemy-roles',unreal.Vector(0,300,90),unreal.Vector(900,0,130))]:
  camera=look+offset;capture.set_actor_location(camera,False,True)
  capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,look),False)
  c.capture_scene();yield delay(.2)
  unreal.RenderingLibrary.export_render_target(world,target,str(out),name+'.png')
 report['passed']=True
iterator=run();deadline=time.monotonic()+120
def finish():
 (out/'preview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play()
 unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending
 try:
  if time.monotonic()>deadline:raise TimeoutError('Cast preview')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
handle=unreal.register_slate_post_tick_callback(tick)
