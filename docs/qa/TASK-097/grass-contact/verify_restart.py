import unreal,json,time,traceback
from pathlib import Path
out=Path(unreal.Paths.project_dir()).resolve()/'.agent-local/qa/TASK-097/grass-contact-restart2-20261008'
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
 report['persisted_grass_types']={}
 for name in ['GT_Meadow','GT_S1_Meadow']:
  asset=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Foliage/'+name)
  values=[v.get_editor_property('cast_contact_shadow') for v in asset.get_editor_property('grass_varieties')]
  report['persisted_grass_types'][name]=values
  assert not any(values),(name,values)
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
 unreal.GameplayStatics.set_global_time_dilation(world,60)
 while not 715 <= clock.get_snapshot().minute_of_day <= 800:yield delay(.1)
 unreal.GameplayStatics.set_global_time_dilation(world,1)
 yield delay(5)
 snap=clock.get_snapshot();report['day_minute']=snap.minute_of_day;report['daylight']=snap.daylight
 home=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/Hearthward.HearthwardHometownFortress'))[0]
 origin=home.get_actor_location();parts=home.get_components_by_class(unreal.StaticMeshComponent)
 floor=next(c.relative_location.z for c in parts if c.get_name()=='BedroomDoorframe')
 camera=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor) if a.actor_has_tag('Task096DayCamera'))
 camera.camera_component.set_field_of_view(78);pc.set_view_target_with_blend(camera,0)
 fire=sorted(home.get_components_by_class(unreal.NiagaraComponent),key=lambda c:c.relative_location.x)[0].get_world_location()
 views=[('bedroom',origin+unreal.Vector(-230,70,floor+180),origin+unreal.Vector(0,550,floor+210)),
        ('gallery',origin+unreal.Vector(720,830,floor+190),origin+unreal.Vector(0,550,floor+190)),
        ('courtyard-fire',fire+unreal.Vector(600,-550,200),fire+unreal.Vector(0,0,40))]
 for name,pos,target in views:
  camera.set_actor_location(pos,False,True);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(pos,target),False)
  yield delay(3)
  unreal.SystemLibrary.execute_console_command(world,'Shot filename='+str(frames/(name+'.png'))+' -nosuffix')
  yield delay(1)
  report['views'].append({'name':name,'position':str(pos),'target':str(target),'minute':clock.get_snapshot().minute_of_day})
 report['grass_assets']=[]
 for c in unreal.ObjectIterator(unreal.StaticMeshComponent):
  mesh=c.static_mesh
  if c.get_world()!=world or not mesh or 'Grass' not in mesh.get_name():continue
  row={'mesh':mesh.get_path_name(),'class':c.get_class().get_name(),'materials':[m.get_path_name() if m else None for m in c.get_materials()]}
  for key in ['cast_shadow','cast_dynamic_shadow','affect_distance_field_lighting','cast_contact_shadow']:
   try:row[key]=str(c.get_editor_property(key))
   except Exception:pass
  if row not in report['grass_assets']:report['grass_assets'].append(row)
 mat=unreal.load_asset('/Game/Hearthward/Assets/NaturalWorld/Rebuild/Materials/M_GrassCards')
 report['grass_material']={key:str(mat.get_editor_property(key)) for key in ['blend_mode','two_sided','shading_model','opacity_mask_clip_value','tangent_space_normal']}
 report['contact_cvar']=unreal.SystemLibrary.get_console_variable_int_value('r.ContactShadows')
 assert report['contact_cvar']==1
 grass=[x for x in report['grass_assets'] if x['class']=='GrassInstancedStaticMeshComponent']
 assert len(grass)==2,grass
 assert all(x['cast_contact_shadow']=='False' for x in grass),grass
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
