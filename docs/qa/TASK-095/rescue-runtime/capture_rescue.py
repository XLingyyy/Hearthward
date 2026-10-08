import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-095/rescue-runtime';out.mkdir(parents=True,exist_ok=True)
frames=out/'frames';frames.mkdir(exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'fixture':'PROTOTYPE_ONLY unsaved paired rescue; production movement, damage, life state and animation','samples':[]}
pending=None
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def xyz(v):return [v.x,v.y,v.z]
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
 actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(350,450,240));capture.tags=['Task095RescueCapture']
 capture.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 light=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(100,200,400));light.tags=['Task095RescueLight'];light.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 wall=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-1000,0,35));wall.tags=['Task095RescueWall']
 wall.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 wall.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
 wall.set_actor_scale3d(unreal.Vector(.15,2,.7));wall.static_mesh_component.set_collision_profile_name('BlockAll')
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(1)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();unreal.GameplayStatics.set_game_paused(world,False)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.RescuePreview')
 player=unreal.GameplayStatics.get_player_character(world,0)
 brother=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Character) if a.actor_has_tag('Task095RescuePatient'))
 cls=unreal.load_class(None,'/Script/Hearthward.HearthwardSurvivalComponent')
 ps=player.get_component_by_class(cls);bs=brother.get_component_by_class(cls)
 capture=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if a.actor_has_tag('Task095RescueCapture'))
 light=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if a.actor_has_tag('Task095RescueLight'))
 light.point_light_component.set_intensity(650);light.point_light_component.set_attenuation_radius(1800)
 target=unreal.RenderingLibrary.create_render_target2d(world,1280,900,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 c=capture.capture_component2d;c.texture_target=target;c.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 c.set_editor_property('capture_every_frame',False);c.set_editor_property('capture_on_movement',False);c.fov_angle=45
 look=unreal.Vector(130,0,65);camera=look+unreal.Vector(300,420,160)
 capture.set_actor_location(camera,False,True);capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,look),False)
 yield delay(3)
 start=unreal.GameplayStatics.get_time_seconds(world);started=False
 while unreal.GameplayStatics.get_time_seconds(world)-start<11:
  t=unreal.GameplayStatics.get_time_seconds(world)-start
  if t>=.6 and not started:
   report['begin_rescue']=ps.begin_rescue(bs);assert report['begin_rescue'];started=True
  a=player.mesh.get_anim_instance();b=brother.mesh.get_anim_instance()
  row={'t':t,'hero_state':str(a.get_editor_property('MotionState')),'brother_state':str(b.get_editor_property('MotionState')),
       'hero':xyz(player.get_actor_location()),'brother':xyz(brother.get_actor_location()),
       'hero_hand':xyz(player.mesh.get_socket_location('hand_r')),'brother_head':xyz(brother.mesh.get_socket_location('head')),
       'brother_life':str(bs.get_editor_property('State').get_editor_property('Life'))}
  report['samples'].append(row);c.capture_scene();yield delay(.06)
  unreal.RenderingLibrary.export_render_target(world,target,str(frames),f'frame-{len(report["samples"]):04d}.png')
 samples=report['samples'];helping=[r for r in samples if r['hero_state']=='Rescue'];rising=[r for r in samples if r['brother_state']=='GetUp']
 assert helping and rising,'Actual paired animation states missing'
 first=helping[0];dist=sum((first['hero'][i]-first['brother'][i])**2 for i in [0,1])**.5
 assert 78<=dist<=83,dist
 assert any(r['brother_state']=='Down' for r in samples)
 assert 'ALIVE' in samples[-1]['brother_life'].upper() and samples[-1]['brother_state'] not in ('Down','GetUp')
 report['approach_distance_cm']=dist
 # Real collision obstruction below the visibility ray, above MaxStepHeight.
 unreal.GameplayStatics.apply_damage(brother,10000,None,None,None)
 player.set_actor_location(unreal.Vector(0,0,92.4),False,True);player.character_movement.stop_movement_immediately()
 yield delay(.3)
 assert ps.begin_rescue(bs)
 wall=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.StaticMeshActor) if a.actor_has_tag('Task095RescueWall'))
 wall.set_actor_location(unreal.Vector(100,0,35),False,True)
 yield delay(2)
 report['blocked_status']=str(ps.get_editor_property('Status'))
 report['blocked_distance_cm']=(player.get_actor_location()-brother.get_actor_location()).length()
 assert '受阻' in report['blocked_status'] and report['blocked_distance_cm']>82,report
 wall.destroy_actor()
 player.set_actor_location(unreal.Vector(0,0,92.4),False,True);player.character_movement.stop_movement_immediately();yield delay(.3)
 assert ps.begin_rescue(bs)
 actions=[a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==player and a.value_type==unreal.InputActionValueType.AXIS2D]
 pairs=[(sub,a) for sub in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(sub.get_outer(),unreal.LocalPlayer)
        for a in actions if any(str(unreal.InputLibrary.key_get_display_name(k))=='W' for k in sub.query_keys_mapped_to_action(a))]
 assert len(pairs)==1,[(str(a),str(sub)) for sub,a in pairs]
 sub,move=pairs[0];sub.inject_input_vector_for_action(move,unreal.Vector(1,0,0),[],[])
 yield delay(.3)
 report['manual_input_status']=str(ps.get_editor_property('Status'))
 assert report['manual_input_status']=='动作已取消',report['manual_input_status']
 report['manual_input_method']='EnhancedInput action injection, not OS keyboard'
 # Complete the interrupted rescue through the production action before swapping roles.
 assert ps.begin_rescue(bs);yield delay(8)
 assert 'ALIVE' in str(bs.get_editor_property('State').get_editor_property('Life')).upper()
 player.set_actor_location(unreal.Vector(0,0,92.4),False,True);player.character_movement.stop_movement_immediately()
 brother.set_actor_location(unreal.Vector(180,0,82.15),False,True);brother.character_movement.stop_movement_immediately()
 unreal.GameplayStatics.apply_damage(player,10000,None,None,None);yield delay(2)
 reverse=[];report['reverse_samples']=reverse
 start=unreal.GameplayStatics.get_time_seconds(world);started=False
 while unreal.GameplayStatics.get_time_seconds(world)-start<11:
  t=unreal.GameplayStatics.get_time_seconds(world)-start
  if t>=.6 and not started:
   assert bs.begin_rescue(ps);started=True
  row={'t':t,'hero_state':str(player.mesh.get_anim_instance().get_editor_property('MotionState')),
       'brother_state':str(brother.mesh.get_anim_instance().get_editor_property('MotionState')),
       'hero':xyz(player.get_actor_location()),'brother':xyz(brother.get_actor_location()),
       'hero_life':str(ps.get_editor_property('State').get_editor_property('Life'))}
  reverse.append(row);c.capture_scene();yield delay(.06)
  unreal.RenderingLibrary.export_render_target(world,target,str(frames),f'reverse-{len(reverse):04d}.png')
 assert any(r['hero_state']=='GetUp' and r['brother_state']=='Rescue' for r in reverse)
 assert 'ALIVE' in reverse[-1]['hero_life'].upper()
 report['passed']=True
iterator=run();deadline=time.monotonic()+180
def finish():
 (out/'preview.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending
 try:
  if time.monotonic()>deadline:raise TimeoutError('Rescue preview')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
handle=unreal.register_slate_post_tick_callback(tick)
