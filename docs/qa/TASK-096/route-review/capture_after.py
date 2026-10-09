import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-096/route-review/after'
out.mkdir(parents=True,exist_ok=True)
frames=out/'frames';frames.mkdir(exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'PROTOTYPE_ONLY isolated PIE visual inspection. New-game entry, camera positioning and accelerated world time for daylight; no asset saves or production settings changed. Not Shipping or natural route acceptance.','views':[]}
pending=None;busy=False;world=None
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def run():
 global world
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
 editor_world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
 editor_world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
 camera=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,25000))
 camera.tags=['Task096DayCamera'];camera.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
 pc=unreal.GameplayStatics.get_player_controller(world,0)
 assert pc.get_hud().get_editor_property('screen').execute_action('new')
 yield delay(10)
 unreal.GameplayStatics.set_game_paused(world,False)
 clock=next(x for x in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if x.get_outer()==world)
 report['initial_minute']=clock.get_snapshot().minute_of_day
 home=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardHometownFortress'))[0]
 origin=home.get_actor_location();parts=home.get_components_by_class(unreal.StaticMeshComponent)
 floor=next(c.relative_location.z for c in parts if c.get_name()=='BedroomDoorframe')
 camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor) if a.actor_has_tag('Task096DayCamera'))
 camera.camera_component.set_field_of_view(78);pc.set_view_target_with_blend(camera,0)
 fire=sorted(home.get_components_by_class(unreal.NiagaraComponent),key=lambda c:c.relative_location.x)[0].get_world_location()
 parts=home.get_components_by_class(unreal.StaticMeshComponent)
 stair=sorted([x for x in parts if x.get_name().startswith('EscapeStair_')],key=lambda c:c.relative_location.y)
 postern=next(x for x in parts if x.get_name().startswith('PosternLintel_'))
 views=[('bedroom',origin+unreal.Vector(-230,70,floor+180),origin+unreal.Vector(0,550,floor+210)),
 ('gallery-west',origin+unreal.Vector(-500,800,floor+170),origin+unreal.Vector(1000,800,floor+140)),
 ('gallery-east',origin+unreal.Vector(1300,850,floor+170),origin+unreal.Vector(0,750,floor+150)),
 ('stair-top',origin+unreal.Vector(1500,950,floor+170),stair[-1].get_world_location()+unreal.Vector(0,0,100)),
 ('stair-bottom',stair[-1].get_world_location()+unreal.Vector(0,250,200),origin+unreal.Vector(1500,1100,floor+100)),
 ('postern',postern.get_world_location()+unreal.Vector(0,-900,-400),postern.get_world_location()+unreal.Vector(0,0,-300))]
 for phase in ['night','noon']:
  if phase=='noon':
   unreal.GameplayStatics.set_global_time_dilation(world,60)
   while clock.get_snapshot().minute_of_day<720 or clock.get_snapshot().minute_of_day>780:yield delay(.1)
   unreal.GameplayStatics.set_global_time_dilation(world,1);yield delay(3)
  for name,pos,target in views:
   camera.set_actor_location(pos,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(pos,target),False)
   yield delay(2)
   unreal.SystemLibrary.execute_console_command(world,'Shot filename='+str(frames/(phase+'-'+name+'.png'))+' -nosuffix')
   yield delay(.6)
   report['views'].append({'name':phase+'-'+name,'position':str(pos),'target':str(target),'minute':clock.get_snapshot().minute_of_day})
 report['passed']=True
iterator=run();deadline=time.monotonic()+300
def finish():
 if world:unreal.GameplayStatics.set_global_time_dilation(world,1)
 (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('day visual capture')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
