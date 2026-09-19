"""Real PIE repair transactions, observed publication, combat reuse and disk rollback."""
import json,time,traceback,shutil
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task024';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'checks':{},'scope':'actual PIE; isolated save pool; explicit damaged-equipment fixture'}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=20):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+5)
def finish():(out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('new_campaign',ui.execute_action('new'));yield delay(1)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    b=player.get_component_by_class(unreal.HearthwardBuildingComponent)
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    timer=player.get_component_by_class(unreal.HearthwardTimedActionComponent)
    store=next(s for s in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if s.get_outer()==world)
    saves=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    companion.set_actor_location(unreal.Vector(-700,-700,90),False,True)
    player.set_actor_location(unreal.Vector(-200,-200,100),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.6)
    check('no_station_rejects_repair_page',not ui.execute_action('page:repairing'))
    check('fixture_build_material',bag.try_add('wood',8)==unreal.HearthwardInventoryResult.SUCCESS)
    b.select_building('workbench');yield delay(.3)
    check('build_workbench',b.confirm_placement());yield wait(lambda:b.building_count()==1,8)
    source=companion.source;remaining=source.get_item_count('wood')
    player.set_actor_location(source.get_owner().get_actor_location()+unreal.Vector(0,90,0),False,True);yield delay(.5)
    interaction=player.get_component_by_class(unreal.HearthwardInteractionComponent)
    for n in range(3):
        interaction.interact_nearest();yield wait(lambda:bag.get_item_count('wood')>=n+1,8)
    check('gathered_real_repair_material',bag.get_item_count('wood')==3 and source.get_item_count('wood')==remaining-3)
    player.set_actor_location(unreal.Vector(-110,-200,100),False,True);yield delay(.5)
    station=b.nearby_workbench();epoch=store.get_timeline_epoch()
    check('equip_axe',game.equip('axe'))
    durability=dict(game.durability);durability['axe']=0;durability['bow']=38
    game.set_editor_property('durability',durability)
    check('broken_axe_has_no_attack_power',game.attack_power()==0)
    check('broken_axe_cannot_attack',not game.attack())
    check('missing_rope_rejected',not b.repair_equipment(station,'axe',epoch))
    check('failed_repair_preserves_all_state',bag.get_item_count('wood')==3 and game.durability.get('axe')==0 and game.events.get('repair:axe',0)==0)
    check('craft_repair_material',b.craft(station,'rope',1,epoch))
    check('real_crafting_to_repair_cost',bag.get_item_count('wood')==2 and bag.get_item_count('rope')==1)
    check('save_broken_equipment',saves.save_point(True));broken=saves.get_points()[-1].save_id
    check('open_crafting',ui.execute_action('page:crafting'))
    check('switch_to_repair',ui.execute_action('page:repairing'))
    check('select_broken_axe',ui.execute_action('repairItem:axe'))
    layout=json.loads(ui.describe_layout())
    check('broken_state_visible',any(x.get('text')=='已损坏' for x in layout['components']))
    for name in ['repairing.items','repairing.details','repairing.actions']:
        check('editable_'+name,ui.set_component_visible(name,False));ui.set_component_visible(name,True)
    check('move_repair_panel',ui.set_component_rect('repairing.details',unreal.Vector2D(755,105),unreal.Vector2D(670,740)))
    check('restore_layout',ui.reload_layout())
    ui.capture_ui('task024-repair-before',1280,720)
    published=[]
    def changed():
        published.append({'wood':bag.get_item_count('wood'),'rope':bag.get_item_count('rope'),'durability':game.durability.get('axe'),
                          'save_blocked':not saves.save_point(True),'reentrant_blocked':not b.repair_equipment(station,'axe',epoch)})
    bag.on_inventory_changed.add_callable(changed)
    start=time.monotonic();check('repair_from_ui',ui.execute_action('repairEquipment'))
    bag.on_inventory_changed.remove_callable(changed)
    check('instant_repair_no_timer',time.monotonic()-start<1 and timer.get_status()!=unreal.HearthwardTimedActionStatus.RUNNING)
    check('single_complete_inventory_notification',len(published)==1 and published[0]['wood']==0 and published[0]['rope']==0 and published[0]['durability']==100)
    check('save_and_reentrant_settlement_blocked',published[0]['save_blocked'] and published[0]['reentrant_blocked'])
    check('repaired_exactly_once',game.durability.get('axe')==100 and game.events.get('repair:axe',0)==1)
    check('unselected_equipment_unchanged',game.durability.get('bow')==38)
    check('full_durability_rejects_repeat',not ui.execute_action('repairEquipment'))
    check('repeat_does_not_record_event',game.events.get('repair:axe',0)==1)
    ui.capture_ui('task024-repair-after',1280,720)
    check('restored_attack_power',game.attack_power()>0)
    check('save_repaired_equipment',saves.save_point(True));repaired=saves.get_points()[-1].save_id
    # Exercise the actual combat path with the restored weapon, then roll back that damage.
    ui.execute_action('back')
    origin=companion.camp.get_actor_location()-unreal.Vector(0,0,100)
    player.set_actor_location(origin+unreal.Vector(900,-600,100),False,True)
    enemy_before=game.opponents.get('guard_1');check('repaired_weapon_attacks',game.attack())
    check('attack_hits_and_consumes_durability',game.opponents.get('guard_1')<enemy_before and game.durability.get('axe')<100)
    player.set_actor_location(unreal.Vector(-110,-200,100),False,True)
    check('combat_rejects_repair',not b.repair_equipment(station,'axe',epoch))
    check('load_broken_node',saves.load_point(broken));yield delay(.3)
    check('rollback_cost_durability_event',bag.get_item_count('wood')==2 and bag.get_item_count('rope')==1 and game.durability.get('axe')==0 and game.events.get('repair:axe',0)==0)
    check('old_epoch_rejected',not b.repair_equipment(station,'axe',epoch))
    check('stale_ui_action_rejected',not ui.execute_action('repairEquipment'))
    epoch=store.get_timeline_epoch()
    check('unsupported_item_rejected',not b.repair_equipment(station,'amulet',epoch))
    check('unknown_item_rejected',not b.repair_equipment(station,'absent',epoch))
    bag.try_remove('axe',1)
    check('missing_equipment_rejected',not b.repair_equipment(station,'axe',epoch));bag.try_add('axe',1)
    check('open_before_distance_change',ui.execute_action('page:repairing'))
    ui.execute_action('repairItem:axe');player.set_actor_location(unreal.Vector(-800,-200,100),False,True)
    check('distance_change_rejected',not ui.execute_action('repairEquipment'))
    player.set_actor_location(unreal.Vector(-110,-200,100),False,True)
    ui.execute_action('back');timer.start_action()
    check('active_action_rejected',not b.repair_equipment(station,'axe',epoch));timer.interrupt_action()
    game.set_editor_property('health',0)
    check('dead_player_rejected',not b.repair_equipment(station,'axe',epoch));game.set_editor_property('health',100)
    check('invalid_requests_leave_cost_intact',bag.get_item_count('wood')==2 and bag.get_item_count('rope')==1 and game.durability.get('axe')==0)
    check('inventory_shortcut_opens_only',ui.execute_action('page:inventory') and ui.execute_action('item:axe') and ui.execute_action('repair') and str(ui.get_page())=='repairing' and game.durability.get('axe')==0)
    check('load_repaired_node',saves.load_point(repaired));yield delay(.3)
    check('restore_full_durability_and_debit',game.durability.get('axe')==100 and bag.get_item_count('wood')==0 and bag.get_item_count('rope')==0)
    check('restore_repair_event',game.events.get('repair:axe',0)==1)
    check('repeated_load',saves.load_point(repaired));yield delay(.3)
    check('no_replayed_repair_event',game.events.get('repair:axe',0)==1 and game.durability.get('axe')==100)
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.5)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('fresh_world_continue_from_disk',ui.execute_action('continue'));yield delay(.4)
    player=unreal.GameplayStatics.get_player_pawn(world,0);game=player.get_component_by_class(unreal.HearthwardGameplayComponent);bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    check('disk_restores_repair_and_inventory',game.durability.get('axe')==100 and game.events.get('repair:axe',0)==1 and bag.get_item_count('wood')==0 and bag.get_item_count('rope')==0)
    check('disk_repair_page_opens',ui.execute_action('page:repairing'))
    check('full_repair_still_rejected',ui.execute_action('repairItem:axe') and not ui.execute_action('repairEquipment'))
    # Empty list remains usable and has no stale submit action.
    for item in ['axe','bow','hood','armor','gloves','boots','belt','shield','quiver']:bag.try_remove(item,bag.get_item_count(item))
    ui.refresh()
    check('empty_repair_list',any(x.get('text')=='背包中没有可维修的装备' for x in json.loads(ui.describe_layout())['components']))
    check('empty_list_submit_rejected',not ui.execute_action('repairEquipment'))
    for name in ['task024-repair-before','task024-repair-after']:
        shutil.copyfile(Path(unreal.Paths.project_saved_dir())/'Task020'/f'{name}.png',out/f'{name}.png')
    report['inventory_notifications']=published;report['passed']=True;finish()

iterator=run();pending=None;deadline=time.monotonic()+150
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('repair overall')
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('repair stage '+str(report['checks']))
            if not pending[0]():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
