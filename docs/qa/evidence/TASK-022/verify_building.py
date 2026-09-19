"""Real PIE building placement, timed settlement and rollback in an isolated save pool."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task022';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'checks':{},'scope':'actual PIE; isolated save pool; no saved map edits'}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=20):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+5)
def finish(): (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

def run():
    editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    platform=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-250,-1500,150))
    platform.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    platform.set_actor_scale3d(unreal.Vector(5,1.6,3));platform.tags=['Task022.SupportTest']
    platform.static_mesh_component.set_collision_profile_name('BlockAll')
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('new_campaign',ui.execute_action('new'));yield delay(1)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    game=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    building=player.get_component_by_class(unreal.HearthwardBuildingComponent)
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    timer=player.get_component_by_class(unreal.HearthwardTimedActionComponent)
    saves=next(s for s in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if s.get_outer()==world)
    companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    # Keep the fixture participants off the construction footprint.
    companion.set_actor_location(unreal.Vector(-700,-700,90),False,True)
    player.set_actor_location(unreal.Vector(-200,-200,100),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.6)
    for item,count in [('wood',32),('stone',16)]:
        check('supply_'+item,bag.try_add(item,count)==unreal.HearthwardInventoryResult.SUCCESS)
    wood=bag.get_item_count('wood');stone=bag.get_item_count('stone')
    check('atomic_recipe_reject',bag.try_consume({'wood':1,'stone':stone+1})==unreal.HearthwardInventoryResult.INSUFFICIENT_ITEMS)
    check('atomic_recipe_no_partial_cost',bag.get_item_count('wood')==wood and bag.get_item_count('stone')==stone)
    check('save_before_build',saves.save_point(True));before=saves.get_points()[-1].save_id
    check('catalog_opens',ui.execute_action('page:building') and str(ui.get_page())=='building')
    check('editable_building_sheet',ui.set_component_rect('building.sheet',unreal.Vector2D(480,100),unreal.Vector2D(760,755)))
    check('reload_layout',ui.reload_layout())
    ui.capture_ui('task022-building-catalog',1280,720)
    check('select_workbench',ui.execute_action('build:workbench'));yield delay(.3)
    check('ground_preview_valid',building.valid_placement)
    check('preview_has_no_cost',bag.get_item_count('wood')==wood and building.building_count()==0)
    yaw=building.yaw;building.rotate_preview();yield delay(.2)
    check('rotate_preview',abs(building.yaw-yaw-15)<.1)
    check('start_build',building.confirm_placement())
    check('reject_double_confirm',not building.confirm_placement())
    check('save_deferred_while_building',not saves.save_point(True))
    yield delay(1)
    check('not_complete_early',building.building_count()==0 and bag.get_item_count('wood')==wood)
    unreal.GameplayStatics.set_game_paused(world,True);elapsed=timer.get_elapsed_seconds();yield delay(.7)
    check('pause_stops_build_time',abs(timer.get_elapsed_seconds()-elapsed)<.02)
    unreal.GameplayStatics.set_game_paused(world,False)
    yield wait(lambda:building.building_count()==1,8)
    check('full_cost_once',bag.get_item_count('wood')==wood-8 and bag.get_item_count('stone')==stone)
    actor=building.get_buildings()[0];position=actor.get_actor_location();rotation=actor.get_actor_rotation()
    check('completed_collision',actor.get_actor_enable_collision())
    check('build_event',game.events.get('build:workbench',0)==1)
    check('save_completed_build',saves.save_point(True));completed=saves.get_points()[-1].save_id
    check('overlap_selection',building.select_building('workbench'));yield delay(.3)
    check('reject_existing_building',not building.valid_placement and not building.confirm_placement())
    building.cancel_placement()
    # A clean second location for interrupt/rollback cases.
    player.set_actor_location(unreal.Vector(-200,100,100),False,True);yield delay(.5)
    building.select_building('campfire');yield delay(.3)
    check('start_for_movement_interrupt',building.confirm_placement());yield delay(.5)
    player.set_actor_location(player.get_actor_location()+unreal.Vector(20,0,0),False,True);yield delay(.2)
    check('movement_interrupt_no_cost',not building.is_building() and bag.get_item_count('wood')==wood-8 and building.building_count()==1)
    building.select_building('campfire');yield delay(.3);check('start_for_damage_interrupt',building.confirm_placement())
    game.apply_damage(1);yield delay(.2)
    check('damage_interrupt_no_cost',not building.is_building() and bag.get_item_count('wood')==wood-8)
    building.select_building('campfire');yield delay(.3);check('start_for_cancel',building.confirm_placement());building.cancel_placement()
    check('cancel_no_cost',not building.is_placing() and bag.get_item_count('stone')==stone)
    building.select_building('campfire');yield delay(.3);check('start_future_build',building.confirm_placement());yield delay(.4)
    check('load_cancels_future',saves.load_point(before));yield delay(5.3)
    check('rollback_removes_buildings',building.building_count()==0 and not building.is_placing())
    check('rollback_restores_materials',bag.get_item_count('wood')==wood and bag.get_item_count('stone')==stone)
    check('load_completed_build',saves.load_point(completed));yield delay(.3)
    restored=building.get_buildings()[0]
    check('restore_exact_transform',(restored.get_actor_location()-position).length()<.01 and abs(restored.get_actor_rotation().yaw-rotation.yaw)<.01)
    check('restored_cost_not_replayed',bag.get_item_count('wood')==wood-8 and bag.get_item_count('stone')==stone and game.events.get('build:workbench',0)==1)
    check('repeat_load',saves.load_point(completed));yield delay(.3)
    check('repeat_load_no_duplicates',len(unreal.GameplayStatics.get_all_actors_with_tag(world,'Hearthward.Building.Completed'))==1)
    # Far outside the configured camp, on the existing Bootstrap floor.
    player.set_actor_location(unreal.Vector(-1700,-1700,100),False,True);yield delay(.6)
    building.select_building('workbench');yield delay(.3)
    check('workbench_rejected_outside_camp',not building.valid_placement and not building.confirm_placement())
    building.cancel_placement();building.select_building('campfire');yield delay(.3)
    check('campfire_allowed_in_wilderness',building.valid_placement)
    check('start_wilderness_campfire',building.confirm_placement());yield wait(lambda:building.building_count()==2,8)
    check('campfire_real_cost',bag.get_item_count('wood')==wood-12 and bag.get_item_count('stone')==stone)
    # Empty footprint has no materials; moving clear of all construction cannot bypass the cost.
    player.set_actor_location(unreal.Vector(-1700,-1350,100),False,True);yield delay(.5)
    bag.try_remove('wood',bag.get_item_count('wood'));building.select_building('campfire');yield delay(.3)
    check('insufficient_material_rejected',not building.valid_placement and not building.confirm_placement())
    building.cancel_placement()
    bag.try_add('wood',4)
    player.set_actor_location(unreal.Vector(-240,-1500,395),False,True);yield delay(.6)
    building.select_building('campfire');yield delay(.3)
    check('footprint_cannot_overhang_ledge',not building.valid_placement and not building.confirm_placement())
    building.cancel_placement()
    player.set_actor_location(unreal.Vector(-10000,-10000,100),False,True);yield delay(.1)
    building.select_building('campfire');yield delay(.2)
    check('empty_ground_rejected',not building.valid_placement and not building.confirm_placement())
    building.cancel_placement()
    player.set_actor_location(unreal.Vector(-500,-500,100),False,True);yield delay(.6)
    building.select_building('campfire');yield delay(.3);check('start_for_jump',building.confirm_placement())
    player.jump();yield delay(.3)
    check('jump_interrupt_no_cost',not building.is_building() and bag.get_item_count('wood')==4)
    player.stop_jumping();yield delay(.8)
    # Capture the built workbench with the game camera, not a simulated drawing.
    player.set_actor_location(unreal.Vector(-300,-200,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.5)
    unreal.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/'completed-workbench.png').as_posix()+'"',pc);yield delay(.5)
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.5)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    check('fresh_world_continue_from_disk',ui.execute_action('continue'));yield delay(.5)
    player=unreal.GameplayStatics.get_player_pawn(world,0)
    building=player.get_component_by_class(unreal.HearthwardBuildingComponent)
    bag=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    check('fresh_world_restores_building',building.building_count()==1 and (building.get_buildings()[0].get_actor_location()-position).length()<.01)
    check('fresh_world_restores_materials',bag.get_item_count('wood')==wood-8)
    check('new_game_resets_buildings',ui.execute_action('new') and building.building_count()==0)
    # No granted materials in this last loop: gather four real units, then spend them on a campfire.
    companion=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HearthwardCompanionFixture)[0]
    source=companion.source;remaining=source.get_item_count('wood')
    player.set_actor_location(source.get_owner().get_actor_location()+unreal.Vector(0,90,0),False,True);yield delay(.5)
    interaction=player.get_component_by_class(unreal.HearthwardInteractionComponent)
    for n in range(4):
        interaction.interact_nearest();yield wait(lambda:bag.get_item_count('wood')>=n+1,8)
    check('gathered_real_build_material',bag.get_item_count('wood')==4 and source.get_item_count('wood')==remaining-4)
    player.set_actor_location(unreal.Vector(-500,-500,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0));yield delay(.5)
    building.select_building('campfire');yield delay(.3)
    check('gather_to_build_starts',building.confirm_placement());yield wait(lambda:building.building_count()==1,8)
    check('gather_to_build_conserves_material',bag.get_item_count('wood')==0 and source.get_item_count('wood')==remaining-4)
    report['passed']=True;finish()

iterator=run();pending=None;deadline=time.monotonic()+180
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('building overall')
        if pending:
            if time.monotonic()>pending[1]:raise TimeoutError('building stage '+str(report['checks']))
            if not pending[0]():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
