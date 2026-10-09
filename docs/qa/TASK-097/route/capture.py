import unreal,json,time,traceback
from pathlib import Path
out=Path(__file__).parent/'visual-02';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'Production natural map, diagnostic repositioning and cameras; existing night lighting; not full campaign playthrough.','nodes':[]}
pending=None;busy=False
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def delay(s):
 end=time.monotonic()+s
 return lambda:time.monotonic()>=end
def ground(w,p):
 hits=unreal.SystemLibrary.line_trace_multi_for_objects(w,unreal.Vector(p.x,p.y,80000),unreal.Vector(p.x,p.y,-30000),[unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1],False,[],unreal.DrawDebugTrace.NONE,True)
 for h in hits:
  b=h.to_tuple()
  if b[9] and ('Landscape' in b[9].get_class().get_name() or b[9].actor_has_tag('Hearthward.NatureGround')) and b[7].z>.65:return b[5]
 return None
def run():
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 for name,parent in [('M_RouteStone','M_RoughStone'),('M_RouteWood','M_OldTimber')]:
  mat=unreal.load_asset('/Game/Hearthward/Assets/TASK-097/Route/'+name)
  unreal.MaterialEditingLibrary.set_material_instance_parent(mat,unreal.load_asset('/Game/Hearthward/Assets/TASK-096/Nearfield/'+parent));unreal.MaterialEditingLibrary.update_material_instance(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
 levels.load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
 unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
 rows=[r for r in json.loads(Path('G:/GameFactory/Hearthward/Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']['locations'] if r['id'].startswith('route_') and r['id']!='route_mine']
 for r in rows:
  for suffix in ['near','far']:
   c=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,25000));c.tags=[r['id']+suffix];c.set_editor_property('is_spatially_loaded',False)
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0)
 assert pc.get_hud().get_editor_property('screen').execute_action('new');yield delay(15)
 unreal.GameplayStatics.set_game_paused(w,False);player=unreal.GameplayStatics.get_player_character(w,0)
 player.character_movement.set_movement_mode(unreal.MovementMode.MOVE_FLYING)
 for r in rows:
  xy=unreal.Vector(r['xy'][0]*100,r['xy'][1]*100,20000);player.set_actor_location(xy,False,True);player.character_movement.stop_movement_immediately();yield delay(4)
  yield lambda:ground(w,xy) is not None
  floor=ground(w,xy);player.set_actor_location(floor+unreal.Vector(300,300,110),False,True);yield delay(3)
  actors=unreal.GameplayStatics.get_all_actors_with_tag(w,'CampaignNode:'+r['id']);assert len(actors)==1,(r['id'],len(actors))
  a=actors[0];mesh=a.get_component_by_class(unreal.StaticMeshComponent)
  assert '/TASK-097/Route/' in mesh.static_mesh.get_path_name()
  assert mesh.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
  assert (a.get_actor_location()-floor).length()<2
  entry={'id':r['id'],'position':str(a.get_actor_location()),'mesh':mesh.static_mesh.get_path_name(),'no_collision':True,'terrain_aligned':True,'views':[]}
  for suffix,offset in [('near',unreal.Vector(250,-320,180)),('far',unreal.Vector(1400,-1700,600))]:
   c=unreal.GameplayStatics.get_all_actors_with_tag(w,r['id']+suffix)[0];eye=floor+offset;c.set_actor_location(eye,False,True);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,floor+unreal.Vector(0,0,80)),False);pc.set_view_target_with_blend(c,0);yield delay(2)
   assert (pc.player_camera_manager.get_camera_location()-eye).length()<5
   unreal.SystemLibrary.execute_console_command(w,'Shot filename='+str(out/(r['id']+'-'+suffix+'.png'))+' -nosuffix');yield delay(.7);entry['views'].append(suffix)
  report['nodes'].append(entry)
 report['passed']=len(report['nodes'])==len(rows)
flow=run();deadline=time.monotonic()+360
def finish():
 (out/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if time.monotonic()>deadline:raise TimeoutError('route landmarks')
  if pending and not pending():return
  pending=next(flow)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
