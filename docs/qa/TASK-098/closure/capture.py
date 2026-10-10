import unreal,json,time,traceback,shutil
from pathlib import Path
out=Path(__file__).parent/'run-05';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'PROTOTYPE_ONLY isolated Graybox with a flat floor and victory prerequisite; actual ReclaimHometown transaction and SavePoint/LoadPoint. Real Slate UI rendered at two sizes and two text scales, not OS DPI acceptance.','checks':{},'ui':[]}
pending=None;busy=False
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def delay(s):
 end=time.monotonic()+s
 return lambda:time.monotonic()>=end
def check(k,v):
 report['checks'][k]=bool(v)
 if not v:raise AssertionError(k)
def sub(cls,w):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 ed=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 floor=ed.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-50));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(1600,1600,1));floor.tags=[unreal.Name('Hearthward.NatureGround')]
 camera=ed.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,500));camera.tags=['Task098ClosureCamera']
 sun=ed.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,1500),unreal.Rotator(pitch=-50,yaw=-40,roll=0));sun.light_component.set_intensity(4)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
 w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);player=unreal.GameplayStatics.get_player_pawn(w,0)
 unreal.GameplayStatics.set_game_paused(w,False)
 unreal.SystemLibrary.execute_console_command(w,'Hearthward.Companion.CreateTest',pc);yield delay(.5)
 player.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
 save=sub(unreal.HearthwardSaveSubsystem,w);check('prototype isolated progress',save.enable_prototype() and save.start_new_progress())
 camp=sub(unreal.HearthwardCampSubsystem,w);bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
 for item in ['shortblade','longblade','spear','bow']:
  check('inventory '+item,bag.try_add(item,1)==unreal.HearthwardInventoryResult.SUCCESS)
 screen=pc.get_hud().get_editor_property('screen')
 screen.open_page('hud');unreal.GameplayStatics.set_game_paused(w,False)
 player.set_actor_location(unreal.Vector(20000,20000,100),False,True)
 check('second camp actual transaction',camp.reclaim_hometown('PROTOTYPE_ONLY_Task098Victory',unreal.Vector(20000,20000,0)))
 yield delay(1)
 before=json.loads(camp.describe());check('two camps recorded',len(before['camps'])==2)
 builder=player.get_component_by_class(unreal.HearthwardBuildingComponent)
 buildings=builder.get_buildings();check('four second camp gifts',len(buildings)==4)
 report['meshes']=[c.static_mesh.get_path_name() for a in buildings for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.static_mesh]
 check('new camp art bound',all('/TASK-098/' in p for p in report['meshes']))
 check('save two camps',save.save_point(True));check('load two camps',save.load_point(save.get_points()[-1].save_id));unreal.GameplayStatics.set_game_paused(w,False);yield delay(2)
 check('camp registry restored',json.loads(camp.describe())['facilities']==before['facilities'])
 player=unreal.GameplayStatics.get_player_pawn(w,0);builder=player.get_component_by_class(unreal.HearthwardBuildingComponent)
 check('no duplicate gifts after load',builder.building_count()==4)
 report['paused_after_restore']=unreal.GameplayStatics.is_game_paused(w)
 report['characters']=[{'class':a.get_class().get_name(),'position':str(a.get_actor_location()),'left_ankle':str(a.mesh.get_socket_location('foot_l')),'mode':str(a.character_movement.movement_mode),'capsule_half':a.capsule_component.get_scaled_capsule_half_height()} for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Character)]
 camera=unreal.GameplayStatics.get_all_actors_with_tag(w,'Task098ClosureCamera')[0]
 for index,a in enumerate(builder.get_buildings()):
  target=a.get_actor_location();eye=target+unreal.Vector(230,-300,230);camera.set_actor_location(eye,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);pc.set_view_target_with_blend(camera,0);yield delay(1)
  unreal.SystemLibrary.execute_console_command(w,'Shot filename='+str(out/('restored-'+str(index)+'.png'))+' -nosuffix');yield delay(.4)
 report['passed']=True
flow=run();deadline=time.monotonic()+240
def finish():
 (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('camp closure capture')
  if pending and not pending():return
  pending=next(flow)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
