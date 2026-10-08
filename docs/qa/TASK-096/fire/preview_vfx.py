import unreal,json,time,traceback,sys
from pathlib import Path
sys.path.insert(0,'G:/GameFactory')
from engine_adapters.ue5.vfx.vfx_functions import spawn_niagara,stop_effect
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-096/vfx-preview';out.mkdir(parents=True,exist_ok=True);frames=out/'frames';frames.mkdir(exist_ok=True)
r={'passed':False,'fixture':'PROTOTYPE_ONLY isolated first article; no production world integration','target_fps':8,'samples':[]}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);ed=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);pending=None;busy=False
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def delay(s):
 t=time.monotonic()+s
 return lambda:time.monotonic()>=t
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Engine.GameModeBase'))
 center=unreal.Vector(0,0,3000)
 floor=ed.spawn_actor_from_class(unreal.StaticMeshActor,center+unreal.Vector(0,0,-10));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(7,7,.2))
 marker=ed.spawn_actor_from_class(unreal.StaticMeshActor,center+unreal.Vector(180,0,50));marker.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));marker.set_actor_scale3d(unreal.Vector(.25,.25,1))
 cam=ed.spawn_actor_from_class(unreal.CameraActor,center+unreal.Vector(430,-550,240));cam.tags=['VFXReviewCamera'];cam.camera_component.set_field_of_view(55);cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(cam.get_actor_location(),center+unreal.Vector(0,0,140)),False)
 light=ed.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,5000),unreal.Rotator(pitch=-45,yaw=-30,roll=0));light.light_component.set_intensity(3)
 for name,z in [('NS_HearthFire',70),('NS_HearthSmoke',100)]:
  actor=spawn_niagara('/Game/Hearthward/Assets/TASK-096/Fire/'+name,(0,0,3000+z),auto_activate=False);actor.tags=['VFXReviewEffect']
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
 camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor) if a.actor_has_tag('VFXReviewCamera'));pc.set_view_target_with_blend(camera,0);yield delay(2)
 effects=[a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.NiagaraActor) if a.actor_has_tag('VFXReviewEffect')];assert len(effects)==2
 r['component_api']=[n for n in dir(unreal.NiagaraComponent) if any(k in n for k in ['particle','active','bound','render','visib','asset'])]
 for a in effects:
  c=a.get_components_by_class(unreal.NiagaraComponent)[0];c.activate(True)
  r.setdefault('components',[]).append({'actor':a.get_path_name(),'location':str(a.get_actor_location()),'component_location':str(c.get_world_location()),'active':c.is_active(),'visible':c.is_visible(),'asset':str(c.get_asset())})
 start=time.monotonic()
 for i in range(80):
  if i==48:
   for a in effects:stop_effect(a,destroy=False)
  unreal.SystemLibrary.execute_console_command(world,'Shot filename='+str(frames/(str(i).zfill(3)+'.png'))+' -nosuffix')
  r['samples'].append({'index':i,'wall_seconds':time.monotonic()-start,'stopped':i>=48});yield delay(.125)
 for a in effects:stop_effect(a,destroy=True)
 r['remaining']=len([a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.NiagaraActor) if a.actor_has_tag('VFXReviewEffect')]);assert r['remaining']==0
 r['passed']=True
iterator=run();deadline=time.monotonic()+180
def finish():
 (out/'report.json').write_text(json.dumps(r,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('VFX first article')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:r['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
