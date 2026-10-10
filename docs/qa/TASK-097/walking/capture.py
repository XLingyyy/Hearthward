import unreal,json,time,traceback,statistics
from pathlib import Path
out=Path(__file__).parent/'run-02';out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'method':'PROTOTYPE_ONLY isolated new game. Grounded movement after diagnostic relocation to each route node; real character camera, no flying. Local frame comparison isolates the nine new decorative landmarks only. No campaign or full-route acceptance.','nodes':[],'performance':[]}
pending=None;busy=False;world=None;moving=None;direction=unreal.Vector(1,0,0);timings=None
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
def xyz(v):return [v.x,v.y,v.z]
def run():
 global world,moving,direction,timings
 unreal.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings').set_editor_property('bThrottleCPUWhenNotForeground',False)
 levels.load_level('/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds')
 unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Hearthward.HearthwardGameMode'))
 levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
 assert pc.get_hud().get_editor_property('screen').execute_action('new');yield delay(12)
 unreal.GameplayStatics.set_game_paused(world,False);player=unreal.GameplayStatics.get_player_character(world,0)
 player.get_component_by_class(unreal.load_class(None,'/Script/Hearthward.HearthwardGameplayComponent')).set_component_tick_enabled(False)
 clock=next(x for x in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if x.get_outer()==world)
 rows=[r for r in json.loads(Path('G:/GameFactory/Hearthward/Resources/Data/gameplay.json').read_text(encoding='utf-8'))['campaign']['locations'] if r['id'].startswith('route_') and r['id']!='route_mine']
 for phase in ['night','day']:
  if phase=='day':
   unreal.GameplayStatics.set_global_time_dilation(world,60)
   while not 720<=clock.get_snapshot().minute_of_day<=780:yield delay(.1)
   unreal.GameplayStatics.set_global_time_dilation(world,1);yield delay(2)
  for r in rows:
   xy=unreal.Vector(r['xy'][0]*100,r['xy'][1]*100,20000)
   player.set_actor_location(xy,False,True);player.character_movement.set_movement_mode(unreal.MovementMode.MOVE_NONE);yield delay(3)
   yield lambda:ground(world,xy) is not None
   node=unreal.GameplayStatics.get_all_actors_with_tag(world,'CampaignNode:'+r['id'])[0]
   start=ground(world,xy+unreal.Vector(0,-1800,0));assert start is not None,r['id']
   player.set_actor_location(start+unreal.Vector(0,0,100),False,True);player.character_movement.set_movement_mode(unreal.MovementMode.MOVE_WALKING)
   direction=unreal.Vector(0,1,0);pc.set_control_rotation(unreal.Rotator(pitch=-12,yaw=90,roll=0));pc.set_view_target_with_blend(player,0)
   yield delay(.5);entry={'id':r['id'],'phase':phase,'start':xyz(player.get_actor_location()),'frames':[]}
   moving=player
   for index in range(4):
    yield delay(1.3)
    entry['frames'].append({'position':xyz(player.get_actor_location()),'velocity':xyz(player.get_velocity()),'movement':str(player.character_movement.movement_mode),'camera':xyz(pc.player_camera_manager.get_camera_location())})
    if index in [0,3]:
     unreal.SystemLibrary.execute_console_command(world,'Shot filename='+str(out/(phase+'-'+r['id']+'-'+str(index)+'.png'))+' -nosuffix');yield delay(.2)
   moving=None;player.character_movement.stop_movement_immediately();report['nodes'].append(entry)
 landmarks=[a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor) if any(str(t).startswith('CampaignNode:route_') for t in a.tags)]
 for visible in [True,False,True]:
  for a in landmarks:a.set_actor_hidden_in_game(not visible)
  yield delay(2);timings=[];yield delay(5);samples=timings;timings=None
  report['performance'].append({'landmarks_visible':visible,'slate_frame_ms':samples,'median_ms':statistics.median(samples),'p95_ms':sorted(samples)[int(len(samples)*.95)]})
 report['passed']=len(report['nodes'])==2*len(rows)
flow=run();deadline=time.monotonic()+480
def finish():
 if world:unreal.GameplayStatics.set_global_time_dilation(world,1)
 (out/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8');unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(delta):
 global pending,busy
 if busy:return
 busy=True
 try:
  if timings is not None:timings.append(delta*1000)
  if moving:moving.add_movement_input(direction,1,True)
  if time.monotonic()>deadline:raise TimeoutError('grounded route sampling')
  if pending and not pending():return
  pending=next(flow)
 except StopIteration:finish()
 except Exception:report['error']=traceback.format_exc();finish()
 finally:busy=False
handle=unreal.register_slate_post_tick_callback(tick)
