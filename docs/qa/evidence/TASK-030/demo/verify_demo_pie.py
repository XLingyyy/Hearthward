import unreal,json,time,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/'Saved/DemoValidation'
report={'ok':False,'checks':{},'resources':[]}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def world():return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def sub(cls):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world())
def check(k,v):
 report['checks'][k]=bool(v);(out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 if not v:raise AssertionError(k)
def wait(pred,secs=60):return pred,time.monotonic()+secs
def delay(secs):
 end=time.monotonic()+secs
 return wait(lambda:time.monotonic()>=end,secs+5)
def targets():return [x for x in unreal.ObjectIterator(unreal.HearthwardHarvestTargetComponent) if x.get_world()==world()]
def settle(p,target,interaction,label):
 pos=target.get_world_location()
 p.set_actor_location(pos+unreal.Vector(165,0,60),False,True)
 p.character_movement.stop_movement_immediately()
 yield delay(1)
 check(label+'_begin',interaction.begin_interaction(target))
 yield wait(lambda:interaction.get_status()!=unreal.HearthwardInteractionStatus.RUNNING,15)
 yield delay(.2)
def build(p,b,ui,kind,offset):
 p.set_actor_location(camp+offset,False,True);p.character_movement.stop_movement_immediately()
 yield delay(1)
 for yaw in [0,90,180,270,45,135,225,315]:
  pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=yaw));b.select_building(kind)
  yield delay(.4)
  if b.valid_placement:break
  b.cancel_placement()
 report[kind+'_placement']=b.feedback
 check(kind+'_valid',b.valid_placement)
 check(kind+'_start',b.confirm_placement())
 yield wait(lambda:not b.is_building(),15)
 check(kind+'_spawned',not b.is_placing())
def run():
 global pc,camp
 levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor,30);yield delay(2)
 unreal.GameplayStatics.get_player_controller(world(),0).get_hud().screen.execute_action('new')
 yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_HearthwardWilds',90);yield delay(10)
 p=unreal.GameplayStatics.get_player_pawn(world(),0);pc=unreal.GameplayStatics.get_player_controller(world(),0);ui=pc.get_hud().screen
 g=p.get_component_by_class(unreal.HearthwardGameplayComponent);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
 interaction=p.get_component_by_class(unreal.HearthwardInteractionComponent);b=p.get_component_by_class(unreal.HearthwardBuildingComponent)
 save=sub(unreal.HearthwardSaveSubsystem);harvest=sub(unreal.HearthwardHarvestSubsystem);store=sub(unreal.HearthwardStorageSubsystem)
 c=unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.HearthwardCompanionFixture)[0];camp=c.camp.get_actor_location()
 check('hero_mesh',p.mesh.skeletal_mesh_asset is not None)
 check('natural_session',save.is_natural_world_enabled())
 # Discover existing authored resources around the camp without placing fake harvest nodes.
 interaction.get_nearest_target();yield delay(1)
 for dx,dy in [(0,0),(1500,0),(-1500,0),(0,1500),(0,-1500),(2500,2000),(-2500,-2000)]:
  p.set_actor_location(camp+unreal.Vector(dx,dy,100),False,True);yield delay(.6);interaction.get_nearest_target()
  if len({str(t.item) for t in targets()})==3:break
 choices={}
 for t in targets():
  k=str(t.item)
  if k not in choices or t.get_world_location().distance(camp)<choices[k].get_world_location().distance(camp):choices[k]=t
 report['resources']=[dict(item=k,key=v.resource_key,position=str(v.get_world_location())) for k,v in choices.items()]
 check('tree_rock_shrub_found',set(choices)=={'wood','stone','herb'})
 for item in ['wood','stone','herb']:
  t=choices[item];before=bag.get_item_count(item)
  yield from settle(p,t,interaction,item)
  check(item+'_real_yield',bag.get_item_count(item)==before+2)
  check(item+'_depleted',harvest.remaining(t.resource_key,t.capacity)==t.capacity-2)
 # Interruption preserves both sides.
 t=choices['wood'];p.set_actor_location(t.get_world_location()+unreal.Vector(165,0,60),False,True);yield delay(1)
 before=bag.get_item_count('wood');left=harvest.remaining(t.resource_key,t.capacity)
 check('interrupt_start',interaction.begin_interaction(t));yield delay(1)
 p.set_actor_location(p.get_actor_location()+unreal.Vector(700,0,0),False,True);yield delay(.8)
 check('interrupted_without_yield',bag.get_item_count('wood')==before and harvest.remaining(t.resource_key,t.capacity)==left)
 # Fill remaining capacity, try gathering, then remove only the added filler.
 filler=int(bag.get_capacity()-bag.get_weight());bag.try_add('stone',filler)
 yield from settle(p,t,interaction,'full_bag')
 check('capacity_preserves_resource',bag.get_item_count('wood')==before and harvest.remaining(t.resource_key,t.capacity)==left)
 bag.try_remove('stone',filler)
 check('save_resources',save.save_point(True));point=save.get_points()[-1].save_id
 yield from settle(p,t,interaction,'after_save')
 check('load_resources',save.load_point(point))
 check('resource_rollback',bag.get_item_count('wood')==before and harvest.remaining(t.resource_key,t.capacity)==left)
 # Collect enough genuine materials for the demo's complete construction loop.
 for item,required in [('wood',26),('stone',7),('herb',2)]:
  while bag.get_item_count(item)<required:
   available=[x for x in targets() if str(x.item)==item and harvest.remaining(x.resource_key,x.capacity)>0]
   check(item+'_supply_available',bool(available))
   t=min(available,key=lambda x:x.get_world_location().distance(camp))
   yield from settle(p,t,interaction,item+'_gather_'+str(bag.get_item_count(item)))
 check('material_source_used',bag.get_item_count('wood')>=26)
 yield from build(p,b,ui,'workbench',unreal.Vector(-400,-400,80))
 station=b.nearby_workbench();check('workbench_usable',station!=unreal.Guid())
 ui.open_page('crafting');check('crafting_ui',str(ui.get_page())=='crafting');ui.open_page('hud')
 for recipe,batches,item,amount in [('rope',2,'rope',2),('arrows',1,'arrow',4),('stone_axe',1,'axe',1),('medicine',1,'medicine',1)]:
  before=bag.get_item_count(item);check('craft_'+recipe,b.craft(station,recipe,batches,store.get_timeline_epoch()));check('output_'+recipe,bag.get_item_count(item)==before+amount)
 check('reject_old_epoch',not b.craft(station,'rope',1,unreal.Guid()))
 yield from build(p,b,ui,'bed',unreal.Vector(350,-400,80))
 bed=next(a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent) for a in b.get_buildings() if a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent) and str(a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent).kind)=='bed')
 g.set_editor_property('health',40);g.set_editor_property('hunger',60);g.set_editor_property('stamina',20)
 yield from settle(p,bed,interaction,'rest')
 check('rest_changes_state',g.health>=69 and g.hunger<51 and g.stamina>=99)
 g.set_editor_property('hunger',10);hp=g.health
 yield from settle(p,bed,interaction,'hungry_rest')
 check('rest_hunger_guard',g.health<hp+10 and g.hunger<11 and '饱食不足' in interaction.get_completion_feedback())
 g.set_editor_property('hunger',80)
 yield from build(p,b,ui,'campfire',unreal.Vector(-350,450,80))
 fire=next(a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent) for a in b.get_buildings() if a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent) and str(a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent).kind)=='campfire')
 meat,wood,roast=[bag.get_item_count(i) for i in ['meat','wood','roast']]
 yield from settle(p,fire,interaction,'cook')
 check('cook_atomic',bag.get_item_count('meat')==meat-1 and bag.get_item_count('wood')==wood-1 and bag.get_item_count('roast')==roast+1)
 check('save_finished_demo',save.save_point(True));point=save.get_points()[-1].save_id
 inventories={i:bag.get_item_count(i) for i in ['wood','stone','herb','rope','axe','medicine','roast']}
 used_key=choices['wood'].resource_key;used_left=harvest.remaining(used_key,12)
 unreal.GameplayStatics.open_level(world(),'/Game/Hearthward/Bootstrap/L_Bootstrap');yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_Bootstrap');yield delay(2)
 menu=unreal.GameplayStatics.get_player_controller(world(),0).get_hud().screen;check('title_load_demo',menu.execute_action('load:'+point.to_string()))
 yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_HearthwardWilds');yield delay(8)
 p=unreal.GameplayStatics.get_player_pawn(world(),0);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent);b=p.get_component_by_class(unreal.HearthwardBuildingComponent)
 check('furniture_restored',b.building_count()==3)
 check('inventory_restored',all(bag.get_item_count(i)==n for i,n in inventories.items()))
 check('depletion_restored',sub(unreal.HearthwardHarvestSubsystem).remaining(used_key,12)==used_left)
 check('bed_interface_restored',sum(bool(a.get_component_by_class(unreal.HearthwardFurnitureInteractionComponent)) for a in b.get_buildings())==2)
 report['demo_inventory']=inventories
 if 'HearthwardDemoHold' in unreal.SystemLibrary.get_command_line():
  (out/'ready.json').write_text(json.dumps(report,ensure_ascii=False),encoding='utf-8')
  yield wait(lambda:(out/'stop').exists(),600)
def finish(error=None):
 if error:report['error']=error
 report['ok']=not error and bool(report['checks']) and all(report['checks'].values())
 (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
flow=run();pending=None
def tick(_dt):
 global pending
 try:
  if pending:
   pred,deadline=pending
   if not pred():
    if time.monotonic()>deadline:raise TimeoutError('wait expired')
    return
  pending=next(flow)
 except StopIteration:finish()
 except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
