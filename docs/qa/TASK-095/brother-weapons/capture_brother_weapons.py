import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-095/brother-weapons';out.mkdir(parents=True,exist_ok=True)
frames=out/'frames';frames.mkdir(exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'fixture':'PROTOTYPE_ONLY unsaved weapon presentation; production timed equipment and attacks','samples':[],'attacks':[]}
pending=None
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def xyz(v):return [v.x,v.y,v.z]
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
 actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(220,230,160));capture.tags=['Task095WeaponCapture']
 capture.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(150,180,320));light.tags=['Task095WeaponLight'];light.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();unreal.GameplayStatics.set_game_paused(world,False)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.CastPreview')
 player=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardCompanionFixture')) if a.actor_has_tag('Task095Cast.Brother'))
 player.set_actor_location(unreal.Vector(0,0,82.4),False,True)
 bag=player.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardInventoryComponent'))
 held=next(c for c in player.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='HeldWeapon')
 capture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('Task095WeaponCapture'))
 light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('Task095WeaponLight'))
 light.point_light_component.set_intensity(700);light.point_light_component.set_attenuation_radius(1500)
 target=unreal.RenderingLibrary.create_render_target2d(world,1280,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 c=capture.capture_component2d;c.texture_target=target;c.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.fov_angle=48
 for item in ['axe','shortblade','shortblade_2','longblade_2','spear_2','blunt_2']:
  bag.try_add(item,1);ident=bag.first_instance(item,False);assert bag.equip_instance(ident),item
  yield delay(1)
  look=unreal.Vector(0,0,90);camera=look+unreal.Vector(260,280,110)
  capture.set_actor_location(camera,False,True);capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,look),False)
  c.capture_scene();yield delay(.15);unreal.RenderingLibrary.export_render_target(world,target,str(frames),item+'-idle.png')
  report['samples'].append({'item':item,'mesh':held.static_mesh.get_name(),'visible':held.is_visible(),'grip':xyz(held.get_world_location()),'hand':xyz(player.mesh.get_socket_location('hand_r')),'scale':xyz(held.get_world_scale())})
  assert held.is_visible(),item
 bag.equip_instance(bag.first_instance('longblade_2',False));yield delay(1)
 for name,offset in [('front',unreal.Vector(350,0,90)),('side',unreal.Vector(0,350,80))]:
  look=player.get_actor_location()+unreal.Vector(0,0,10);camera=look+offset
  capture.set_actor_location(camera,False,True);capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,look),False)
  c.capture_scene();yield delay(.15);unreal.RenderingLibrary.export_render_target(world,target,str(frames),'longblade-'+name+'.png')
 report['passed']=True
iterator=run();deadline=time.monotonic()+180
def finish():
 (out/'preview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending
 try:
  if time.monotonic()>deadline:raise TimeoutError('Weapon preview')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
handle=unreal.register_slate_post_tick_callback(tick)
