"""Actual PIE workbench access, instant inventory settlement and disk restore."""
import json,time,traceback,shutil
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task025Rev2';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'checks':{},'scope':'actual PIE; isolated save pool; no saved map edits'}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=20):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+5)
def finish():(out/'playercraft.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    obstacle=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-1500,-1500,100))
    obstacle.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    obstacle.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    obstacle.set_actor_scale3d(unreal.Vector(.1,2,2));obstacle.tags=['Task023.AccessWall']
    obstacle.static_mesh_component.set_collision_profile_name('BlockAll')
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('new_campaign',ui.execute_action('new'));yield delay(1)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    building=player.get_component_by_class(unreal.HearthwardBuildingComponent)
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    timer=player.get_component_by_class(unreal.HearthwardTimedActionComponent)
    store=next(s for s in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if s.get_outer()==world)
    saves=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    companion.set_actor_location(unreal.Vector(-700,-700,90),False,True)
    player.set_actor_location(unreal.Vector(-200,-200,100),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.6)
    check('no_workbench_rejects_page',not ui.execute_action('page:crafting'))
    check('supply_build_material',bag.try_add('wood',8)==unreal.HearthwardInventoryResult.SUCCESS)
    check('select_workbench',building.select_building('workbench'));yield delay(.3)
    check('start_actual_construction',building.confirm_placement());yield wait(lambda:building.building_count()==1,8)
    check('construction_consumed_wood',bag.get_item_count('wood')==0)
    # Obtain crafting materials through the existing five-second gathering interaction.
    source=companion.source;remaining=source.get_item_count('wood')
    player.set_actor_location(source.get_owner().get_actor_location()+unreal.Vector(0,90,0),False,True);yield delay(.5)
    interaction=player.get_component_by_class(unreal.HearthwardInteractionComponent)
    for n in range(3):
        interaction.interact_nearest();yield wait(lambda:bag.get_item_count('wood')>=n+1,8)
    check('real_gathering_materials',bag.get_item_count('wood')==3 and source.get_item_count('wood')==remaining-3)
    player.set_actor_location(unreal.Vector(-110,-200,100),False,True);yield delay(.6)
    station=building.nearby_workbench();epoch=store.get_timeline_epoch()
    check('completed_station_accessible',building.can_use_workbench(station))
    check('save_before_crafting',saves.save_point(True));before=saves.get_points()[-1].save_id
    arrows=bag.get_item_count('arrow');ropes=bag.get_item_count('rope');weight=bag.get_weight()
    check('open_crafting',ui.execute_action('page:crafting') and str(ui.get_page())=='crafting')
    check('menu_pauses_world',unreal.GameplayStatics.is_game_paused(world))
    for name in ['crafting.recipes','crafting.details','crafting.actions']:
        check('editable_'+name,ui.set_component_visible(name,False));ui.set_component_visible(name,True)
    check('move_details',ui.set_component_rect('crafting.details',unreal.Vector2D(760,105),unreal.Vector2D(670,740)))
    check('restore_default_layout',ui.reload_layout())
    check('select_arrows',ui.execute_action('recipe:arrows'))
    check('batch_increment',ui.execute_action('craftMore'))
    ui.capture_ui('task023-crafting',1280,720)
    start=time.monotonic();check('instant_batch',ui.execute_action('craft'))
    check('no_five_second_timer',time.monotonic()-start<1 and timer.get_status()!=unreal.HearthwardTimedActionStatus.RUNNING)
    check('two_batch_cost_and_output',bag.get_item_count('wood')==1 and bag.get_item_count('arrow')==arrows+8)
    check('correct_net_weight',abs(bag.get_weight()-(weight-1.6))<.001)
    check('craft_event',game.events.get('craft:arrows',0)==2)
    check('insufficient_batch_rejected',not ui.execute_action('craft'))
    check('failed_batch_no_partial_inventory',bag.get_item_count('wood')==1 and bag.get_item_count('arrow')==arrows+8)
    check('failure_does_not_record_event',game.events.get('craft:arrows',0)==2)
    check('select_resets_batch',ui.execute_action('recipe:rope') and ui.execute_action('craft'))
    check('second_recipe_real_output',bag.get_item_count('wood')==0 and bag.get_item_count('rope')==ropes+1)
    check('ordinary_recipe_requires_no_blueprint',game.events.get('craft:rope',0)==1)
    ui.capture_ui('task023-insufficient-material',1280,720)
    check('save_after_crafting',saves.save_point(True));after=saves.get_points()[-1].save_id
    check('load_before_crafting',saves.load_point(before));yield delay(.4)
    check('rollback_inventory',bag.get_item_count('wood')==3 and bag.get_item_count('arrow')==arrows and bag.get_item_count('rope')==ropes)
    check('rollback_events',game.events.get('craft:arrows',0)==0 and game.events.get('craft:rope',0)==0)
    check('load_closes_crafting_page',str(ui.get_page())=='hud')
    check('stale_page_action_rejected',not ui.execute_action('craft'))
    check('old_epoch_rejected',not building.craft(station,'arrows',1,epoch))
    epoch=store.get_timeline_epoch()
    check('restored_station_id_valid',building.can_use_workbench(station))
    check('invalid_recipe_rejected',not building.craft(station,'absent',1,epoch))
    check('zero_batch_rejected',not building.craft(station,'arrows',0,epoch))
    check('negative_batch_rejected',not building.craft(station,'arrows',-1,epoch))
    check('oversized_batch_rejected',not building.craft(station,'arrows',2147483647,epoch))
    check('open_before_distance_change',ui.execute_action('page:crafting'))
    player.set_actor_location(unreal.Vector(-600,-200,100),False,True)
    check('distance_change_rejected',not ui.execute_action('craft') and not building.can_use_workbench(station))
    player.set_actor_location(unreal.Vector(-110,-200,100),False,True)
    wall=unreal.GameplayStatics.get_all_actors_with_tag(world,'Task023.AccessWall')[0]
    wall.set_actor_location(unreal.Vector(-50,-200,100),False,True)
    check('wall_blocks_workbench',not building.can_use_workbench(station) and not ui.execute_action('craft'))
    wall.set_actor_location(unreal.Vector(-1500,-1500,100),False,True)
    check('clear_path_allows_workbench',building.can_use_workbench(station))
    ui.execute_action('back')
    timer.start_action()
    check('active_action_rejects_workbench',not building.craft(station,'arrows',1,epoch))
    timer.interrupt_action()
    game.set_editor_property('health',0)
    check('dead_player_rejected',not building.craft(station,'arrows',1,epoch))
    game.set_editor_property('health',100)
    check('all_invalid_access_preserves_material',bag.get_item_count('wood')==3 and bag.get_item_count('arrow')==arrows)
    check('reload_completed_crafting',saves.load_point(after));yield delay(.3)
    check('restore_exact_products',bag.get_item_count('wood')==0 and bag.get_item_count('arrow')==arrows+8 and bag.get_item_count('rope')==ropes+1)
    check('restore_exact_events',game.events.get('craft:arrows',0)==2 and game.events.get('craft:rope',0)==1)
    check('repeated_load',saves.load_point(after));yield delay(.3)
    check('no_duplicate_products',bag.get_item_count('arrow')==arrows+8 and bag.get_item_count('rope')==ropes+1)
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.5)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('fresh_world_continue',ui.execute_action('continue'));yield delay(.5)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    building=player.get_component_by_class(unreal.HearthwardBuildingComponent)
    check('disk_products_preserved',bag.get_item_count('arrow')==arrows+8 and bag.get_item_count('rope')==ropes+1)
    check('disk_workbench_usable',building.can_use_workbench(station))
    check('new_game_clears_workbenches',ui.execute_action('new') and building.building_count()==0)
    check('old_station_rejected_after_new_game',not building.can_use_workbench(station))
    for name in ['task023-crafting','task023-insufficient-material']:
        shutil.copyfile(Path(unreal.Paths.project_saved_dir())/'Task020'/f'{name}.png',out/f'{name}.png')
    report['passed']=True;finish()

iterator=run();pending=None;deadline=time.monotonic()+150
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('crafting overall')
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('crafting stage '+str(report['checks']))
            if not pending[0]():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
