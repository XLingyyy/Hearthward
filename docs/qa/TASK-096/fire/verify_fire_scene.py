import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-096/fire-integration'
out.mkdir(parents=True,exist_ok=True);frames=out/'frames';frames.mkdir(exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'Natural map, new-game UI entry, isolated save pool, actual LoadPoint twice; camera and distance positioning are test instrumentation, not natural route acceptance','checks':{},'views':[]}
pending=None;busy=False
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def delay(seconds):
 end=time.monotonic()+seconds
 return lambda:time.monotonic()>=end
def check(name,value):
 report['checks'][name]=bool(value)
 assert value,name
def subsystem(cls,world):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
def home_in(world):
 homes=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardHometownFortress'))
 check('exactly one fortress',len(homes)==1)
 return homes[0]
def inspect(world,name):
 home=home_in(world)
 fx=home.get_components_by_class(unreal.NiagaraComponent)
 lights=[c for c in home.get_components_by_class(unreal.PointLightComponent) if c.component_has_tag('HearthwardRaidLight')]
 all_fx=[c for c in unreal.ObjectIterator(unreal.NiagaraComponent) if c.get_world()==world and c.component_has_tag('HearthwardRaidVFX')]
 all_lights=[c for c in unreal.ObjectIterator(unreal.PointLightComponent) if c.get_world()==world and c.component_has_tag('HearthwardRaidLight')]
 report.setdefault('counts',{})[name]={'vfx':len(fx),'lights':len(lights),'all_world_vfx':len(all_fx),'all_world_lights':len(all_lights),'home':home.get_path_name()}
 check(name+' no orphan effects or lights',len(all_fx)==len(fx) and len(all_lights)==len(lights))
 return home,len(fx),len(lights)
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
 world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
 editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 camera=editor.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,25000));camera.tags=['Task096FireCamera'];camera.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
 check('new-game action',pc.get_hud().get_editor_property('screen').execute_action('new'));yield delay(10)
 unreal.GameplayStatics.set_game_paused(world,False);yield delay(2)
 home,n,l=inspect(world,'new');check('new game has three bounded pockets',n==6 and l==3)
 player=unreal.GameplayStatics.get_player_pawn(world,0);origin=home.get_actor_location()
 camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor) if a.actor_has_tag('Task096FireCamera'))
 camera.camera_component.set_field_of_view(78);pc.set_view_target_with_blend(camera,0)
 wood=[c for c in home.get_components_by_class(unreal.StaticMeshComponent) if c.component_has_tag('RaidBurningWood')]
 check('decorative fuel never blocks',len(wood)==9 and all(c.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION for c in wood))
 fire=sorted(home.get_components_by_class(unreal.NiagaraComponent),key=lambda c:c.relative_location.x)[0].get_world_location()
 floor=next(c.relative_location.z for c in home.get_components_by_class(unreal.StaticMeshComponent) if c.get_name()=='BedroomDoorframe')
 for label,pos,target in [('gallery',origin+unreal.Vector(720,850,floor+185),fire+unreal.Vector(0,0,60)),('courtyard',fire+unreal.Vector(600,-550,200),fire+unreal.Vector(0,0,40)),('postern',origin+unreal.Vector(0,5800,floor+200),origin+unreal.Vector(2850,4700,floor))]:
  camera.set_actor_location(pos,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(pos,target),False);yield delay(3)
  unreal.SystemLibrary.execute_console_command(world,'Shot filename='+str(frames/(label+'.png'))+' -nosuffix');yield delay(.5)
  report['views'].append({'name':label,'position':str(pos),'target':str(target)})
 save=subsystem(unreal.HearthwardSaveSubsystem,world);points=save.get_points();check('real save point exists',bool(points));point=points[0].save_id
 for i in range(2):
  check('LoadPoint '+str(i),save.load_point(point));yield delay(4)
  home,n,l=inspect(world,'reload-'+str(i));check('reload bounded '+str(i),n==6 and l==3)
 player=unreal.GameplayStatics.get_player_pawn(world,0);original=player.get_actor_location()
 player.set_actor_location(home.get_actor_location()+unreal.Vector(25000,0,3000),False,True);yield delay(2)
 home,n,l=inspect(world,'far');check('distance cleanup',n==0 and l==0)
 player.set_actor_location(original,False,True);yield delay(2)
 home,n,l=inspect(world,'returned');check('return bounded',n==6 and l==3)
 report['passed']=True
iterator=run();deadline=time.monotonic()+300
def finish():
 (out/'scene.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('Fire scene validation')
  if pending and not pending():return
  pending=next(iterator)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
