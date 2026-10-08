"""Isolated visual fixture using the real campaign actor and shooting code."""
import unreal,json,time,traceback
from pathlib import Path
out=Path('G:/GameFactory/Hearthward/.agent-local/qa/TASK-095/archer-runtime/pie');out.mkdir(parents=True,exist_ok=True)
(out/'frames').mkdir(exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'fixture':'PROTOTYPE_ONLY isolated QA; production Campaign actor; unsaved level','samples':[]}
active={};pending=None;frame=0;last_capture=0;capture_pending=False
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/Tests/Graybox/L_GrayboxValidation')
 actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 for cls,label in [(unreal.SceneCapture2D,'Task095Capture'),(unreal.PointLight,'Task095Light')]:
  actor=actors.spawn_actor_from_class(cls,unreal.Vector(200,-200,350));actor.tags=[label];actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(1)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
 pc=unreal.GameplayStatics.get_player_controller(world,0);player=unreal.GameplayStatics.get_player_pawn(world,0)
 unreal.GameplayStatics.set_game_paused(world,False)
 game=player.get_component_by_class(unreal.HearthwardGameplayComponent);game.set_editor_property('enabled',True)
 player.set_actor_location(unreal.Vector(1000,0,100),False,True)
 unreal.SystemLibrary.execute_console_command(world,'Hearthward.Test095.ArcherPreview',pc)
 archer=next(x for x in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCampaignActor) if x.actor_has_tag('Task095ArcherPreview'))
 assert archer.mesh.skeletal_mesh_asset.get_name()=='SK_Archer_Combat'
 report['materials']=[m.get_path_name() if m else None for m in archer.mesh.get_materials()]
 assert len(report['materials'])==6 and all(p and p.startswith('/Game/') for p in report['materials']),report['materials']
 unreal.SystemLibrary.execute_console_command(world,'Slate.bAllowThrottling 0',pc)
 unreal.SystemLibrary.execute_console_command(world,'t.IdleWhenNotForeground 0',pc)
 capture=next(x for x in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SceneCapture2D) if x.actor_has_tag('Task095Capture'))
 light=next(x for x in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PointLight) if x.actor_has_tag('Task095Light'))
 target=unreal.RenderingLibrary.create_render_target2d(world,960,720,unreal.TextureRenderTargetFormat.RTF_RGBA8)
 capture.capture_component2d.texture_target=target;capture.capture_component2d.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 capture.capture_component2d.set_editor_property('fov_angle',40)
 capture.capture_component2d.set_editor_property('capture_every_frame',False);capture.capture_component2d.set_editor_property('capture_on_movement',False)
 report['show_flags']=str(capture.capture_component2d.get_editor_property('show_flag_settings'))
 light.point_light_component.set_intensity(600);light.point_light_component.set_attenuation_radius(2000)
 location=archer.get_actor_location();camera=location+unreal.Vector(340,-440,160)
 capture.set_actor_location(camera,False,True);capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera,location+unreal.Vector(0,0,30)),False)
 light.set_actor_location(location+unreal.Vector(180,-220,250),False,True)
 unreal.GameplayStatics.set_game_paused(world,True)
 yield delay(5)
 capture.capture_component2d.capture_source=unreal.SceneCaptureSource.SCS_BASE_COLOR
 c=capture.capture_component2d;c.capture_scene();yield delay(.2)
 unreal.RenderingLibrary.export_render_target(world,target,str(out),'base-color.png')
 capture.capture_component2d.capture_source=unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 unreal.GameplayStatics.set_game_paused(world,False)
 active.update(world=world,archer=archer,capture=capture,target=target,started=time.monotonic(),game=game)
 yield delay(9)
 report['passed']=any(s['arrow_count']>0 for s in report['samples']) and any(s['motion']=='Attack' for s in report['samples'])
iterator=run();deadline=time.monotonic()+120
def finish():
 report['samples']=report['samples'][:frame]
 (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play()
 unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,frame,last_capture,capture_pending
 try:
  now=time.monotonic()
  if now>deadline:raise TimeoutError('PIE visual fixture')
  if active:
   w=active['world'];c=active['capture'];actor=active['archer'];anim=actor.mesh.get_anim_instance()
   if capture_pending:
    unreal.RenderingLibrary.export_render_target(w,active['target'],str(out/'frames'),f'{frame:04d}.png');frame+=1;capture_pending=False
   if now-last_capture>=1/12:
    last_capture=now;c.capture_component2d.capture_scene();capture_pending=True
    report['samples'].append({'frame':frame,'elapsed':now-active['started'],'motion':str(anim.motion_state),
     'arrow_count':len(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardProjectile)),
     'health':active['game'].get_editor_property('health')})
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:
  report['error']=traceback.format_exc();finish()
handle=unreal.register_slate_post_tick_callback(tick)
