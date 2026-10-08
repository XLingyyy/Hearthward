import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-095/weapons';out.mkdir(parents=True,exist_ok=True)
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
 player=unreal.GameplayStatics.get_player_character(world,0)
 player.set_actor_location(unreal.Vector(0,0,92.4),False,True);player.set_actor_rotation(unreal.Rotator(),False)
 game=player.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardGameplayComponent'));game.set_editor_property('Enabled',True)
 bag=player.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardInventoryComponent'))
 combat=player.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardCombatComponent'))
 held=next(c for c in player.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='HeldWeapon')
 capture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('Task095WeaponCapture'))
 light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('Task095WeaponLight'))
 light.point_light_component.set_intensity(700);light.point_light_component.set_attenuation_radius(1500)
 target=unreal.RenderingLibrary.create_render_target2d(world,1280,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 c=capture.capture_component2d;c.texture_target=target;c.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.fov_angle=48
 for item in ['spear_2']:
  bag.try_add(item,1);ident=bag.first_instance(item,False);assert game.equip_instance(ident),item
  yield delay(.8)
  look=unreal.Vector(0,0,95);camera=look+unreal.Vector(260,280,110)
  capture.set_actor_location(camera,False,True);capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,look),False)
  c.capture_scene();yield delay(.15)
  unreal.RenderingLibrary.export_render_target(world,target,str(frames),item+'-idle.png')
  report['samples'].append({'item':item,'mesh':held.static_mesh.get_name(),'grip':xyz(held.get_world_location()),'hand':xyz(player.mesh.get_socket_location(held.get_attach_socket_name())),'scale':xyz(held.get_world_scale())})
  if item in ['shortblade_2','longblade_2','spear_2','blunt_2']:
   report[item+'_attack']=combat.attack(False)
   for i in range(8):
    report['attacks'].append({'item':item,'heavy':False,'frame':i,'time':unreal.GameplayStatics.get_time_seconds(world),'elapsed':combat.get_editor_property('Elapsed'),'action':str(combat.get_editor_property('Action')),'grip':xyz(held.get_world_location()),'axis':xyz(held.get_up_vector())})
    c.capture_scene();yield delay(.05)
    unreal.RenderingLibrary.export_render_target(world,target,str(frames),item+f'-attack-{i:02d}.png')
   yield delay(.5)
   if item=='spear_2':
    report['spear_heavy']=combat.attack(True)
    for i in range(12):
     report['attacks'].append({'item':item,'heavy':True,'frame':i,'time':unreal.GameplayStatics.get_time_seconds(world),'elapsed':combat.get_editor_property('Elapsed'),'action':str(combat.get_editor_property('Action')),'grip':xyz(held.get_world_location()),'axis':xyz(held.get_up_vector())})
     c.capture_scene();yield delay(.05)
     unreal.RenderingLibrary.export_render_target(world,target,str(frames),item+f'-heavy-{i:02d}.png')
    yield delay(.5)
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
