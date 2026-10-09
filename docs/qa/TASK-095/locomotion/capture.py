import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-095/locomotion/calibrated-03';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'fixture':'PROTOTYPE_ONLY isolated movement sampling; calibrated production animation','samples':[]}
pending=None;busy=False;moving=None;direction=unreal.Vector(1,0,0)
def xyz(v):return [v.x,v.y,v.z]
def delay(s):
 end=time.monotonic()+s
 return lambda:time.monotonic()>=end
def run():
 global moving
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
 actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 camera=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(0,0,0));camera.tags=['MotionCapture'];camera.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(0,0,300));light.tags=['MotionLight'];light.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,-3500,-55));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(160,30,1))
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();unreal.GameplayStatics.set_game_paused(world,False)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.CastPreview')
 hero=unreal.GameplayStatics.get_player_pawn(world,0)
 hero.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardGameplayComponent')).set_component_tick_enabled(False)
 brother=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardCompanionFixture')) if a.actor_has_tag('Task095Cast.Brother'))
 for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Character):a.set_actor_location(unreal.Vector(-5000,1000,100),False,True)
 camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('MotionCapture'))
 light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('MotionLight'))
 light.point_light_component.set_intensity(220);light.point_light_component.set_attenuation_radius(1500)
 target=unreal.RenderingLibrary.create_render_target2d(world,800,600,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 c=camera.capture_component2d;c.texture_target=target;c.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.fov_angle=50
 for role,actor,speeds in [('Hero',hero,[90,140,180,250,350,600]),('Brother',brother,[90,140,180,250,350,600])]:
  movement=actor.character_movement;movement.set_editor_property('run_physics_with_no_controller',True)
  actor.mesh.set_editor_property('visibility_based_anim_tick_option',unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
  for speed in speeds:
   label=f'{role}-{speed}';folder=out/label;folder.mkdir(exist_ok=True)
   actor.set_actor_location(unreal.Vector(-2000,-3500,100),False,True);actor.set_actor_rotation(unreal.Rotator(0,0,0),False)
   movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING);movement.max_walk_speed=speed;moving=actor
   yield delay(.6)
   for i in range(48):
    yield delay(.025)
    loc=actor.get_actor_location();anim=actor.mesh.get_anim_instance()
    report['samples'].append({'label':label,'frame':i,'time':unreal.GameplayStatics.get_time_seconds(world),'position':xyz(loc),'velocity':xyz(actor.get_velocity()),'left':xyz(actor.mesh.get_socket_location('foot_l')),'right':xyz(actor.mesh.get_socket_location('foot_r')),'motion':str(anim.get_editor_property('motion_state'))})
    if i%4==0:
     look=loc+unreal.Vector(0,0,-15);pos=look+unreal.Vector(160,320,70)
     camera.set_actor_location(pos,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(pos,look),False)
     light.set_actor_location(loc+unreal.Vector(100,150,220),False,True)
     c.capture_scene();yield delay(.01);unreal.RenderingLibrary.export_render_target(world,target,str(folder),f'{i:03}.png')
   moving=None;movement.stop_movement_immediately();yield delay(.3)
  actor.set_actor_location(unreal.Vector(-5000,1000,100),False,True)
 report['passed']=True
iterator=run();deadline=time.monotonic()+240
def finish():
 (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if moving:moving.add_movement_input(direction,1,True)
  if time.monotonic()>deadline:raise TimeoutError('locomotion capture')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
