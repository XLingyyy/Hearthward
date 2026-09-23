"""Natural-map production New Game -> real model -> execution -> save/load.

Run in an isolated HearthwardSaveTestPool; never saves editor map assets.
"""
import json
import time
import traceback
import re
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
out = root / 'Saved/NaturalCampValidation'
out.mkdir(parents=True, exist_ok=True)
workshop_only = 'HearthwardCampWorkshopTest' in unreal.SystemLibrary.get_command_line()
report = {'ok': False, 'checks': {}, 'steps': []}
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def check(name, value):
    report['checks'][name] = bool(value)
    (out / 'progress.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if not value:
        raise AssertionError(name)

def wait(pred, seconds=90):
    return pred, time.monotonic() + seconds

def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)

def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()

def sub(cls):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world())

def snapshot(label, c, ai):
    report['steps'].append(dict(label=label, position=str(c.get_actor_location()),
        phase=str(c.get_phase()), reason=c.block_reason, requested=c.get_requested(),
        delivered=c.get_delivered(), acquired=c.get_acquired(), carried=c.get_carried(),
        source=c.source.get_item_count('wood'), raw=ai.get_last_structured_result(), status=ai.get_status()))

def run():
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor, 30)
    yield delay(2)
    pc = unreal.GameplayStatics.get_player_controller(world(), 0)
    pc.get_hud().screen.execute_action('new')
    yield wait(lambda: unreal.GameplayStatics.get_current_level_name(world(), True) == 'L_HearthwardWilds', 90)
    yield delay(8)
    w = world()
    p = unreal.GameplayStatics.get_player_pawn(w, 0)
    ui = unreal.GameplayStatics.get_player_controller(w, 0).get_hud().screen
    save = sub(unreal.HearthwardSaveSubsystem)
    check('natural_session', save.is_natural_world_enabled())
    check('new_campaign', save.get_campaign_id() != unreal.Guid())
    companions = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.HearthwardCompanionFixture)
    check('single_companion', len(companions) == 1)
    c = companions[0]
    g = p.get_component_by_class(unreal.HearthwardGameplayComponent)
    ai = sub(unreal.HearthwardLocalAISubsystem)
    store = sub(unreal.HearthwardStorageSubsystem)
    check('gameplay_enabled', g.enabled)
    check('initial_resource', c.source.get_item_count('wood') == 16)
    check('dialogue_range', c.can_communicate(p))
    if 'HearthwardCampLegacyTest' in unreal.SystemLibrary.get_command_line():
        yield from legacy_natural(p, save, store)
        return
    if workshop_only:
        yield from workshop(w, p, c, ui, save, ai, store, g)
        return
    ui.open_page('dialogue')
    check('dialogue_open', str(ui.get_page()) == 'dialogue')
    check('real_model_submit', ai.submit_player_text(p, c, '帮我采集两份木材带回营地。'))
    yield wait(lambda: not ai.is_busy(), 180)
    snapshot('proposal', c, ai)
    check('model_candidate', ai.has_candidate())
    check('confirm', ai.confirm_candidate(ai.get_candidate_id()))
    ui.open_page('hud')
    yield wait(lambda: c.get_acquired() >= 1, 90)
    snapshot('mid_task', c, ai)
    before = {x.save_id.to_string() for x in save.get_points()}
    check('save_mid_task', save.save_point(True))
    point = next(x.save_id for x in save.get_points() if x.save_id.to_string() not in before)
    acquired, delivered = c.get_acquired(), c.get_delivered()
    stock, storage = c.source.get_item_count('wood'), store.get_item_count('wood')
    yield wait(lambda: c.get_delivered() == 2, 90)
    check('gather_settlement', c.source.get_item_count('wood') == 14 and store.get_item_count('wood') == 2)
    check('load_mid_task', save.load_point(point))
    check('restored_counters', c.get_acquired() == acquired and c.get_delivered() == delivered)
    check('restored_inventories', c.source.get_item_count('wood') == stock and store.get_item_count('wood') == storage)
    yield wait(lambda: c.get_delivered() == 2, 90)
    check('no_duplicate_settlement', c.source.get_item_count('wood') == 14 and store.get_item_count('wood') == 2)
    snapshot('restored_completed', c, ai)
    check('follow', g.order_companion('follow'))
    yield delay(3)
    check('hold', g.order_companion('wait'))
    ui.open_page('memory')
    check('memory_open', str(ui.get_page()) == 'memory')
    ui.open_page('hud')
    ui.capture_ui('natural-camp-completed', 1280, 720)
    check('new_game_again', save.start_new_progress())
    check('new_game_resets_resources', c.source.get_item_count('wood') == 16 and store.get_item_count('wood') == 0)
    check('new_game_resets_command', c.get_requested() == 0)

def workshop(w, p, c, ui, save, ai, store, g):
    goal = unreal.HearthwardAgentGoal()
    for key, value in dict(intent='collect', item='wood', quantity=10,
                           quantity_mode='additional_acquired', source_ref='S1').items():
        goal.set_editor_property(key, value)
    check('collect_building_materials', ai.set_structured_goal(p, c, goal))
    check('confirm_materials', ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: c.get_delivered() == 10, 150)
    check('materials_delivered', store.get_item_count('wood') == 10)
    g.order_companion('wait')
    p.set_actor_location(c.camp.get_actor_location() + unreal.Vector(100, 0, 30), False, True)
    yield delay(1)
    check('storage_open', ui.execute_action('page:storage'))
    ui.execute_action('withdraw:wood')
    ui.execute_action('quantity:7')
    check('withdraw_eight', ui.execute_action('transfer'))
    ui.open_page('hud')
    pc = unreal.GameplayStatics.get_player_controller(w, 0)
    building = p.get_component_by_class(unreal.HearthwardBuildingComponent)
    # Try the four directions on the authored camp plateau without bypassing placement checks.
    for yaw in (0, 90, 180, 270):
        pc.set_control_rotation(unreal.Rotator(pitch=-20, yaw=yaw))
        building.select_building('workbench')
        yield delay(.5)
        if building.valid_placement:
            break
        building.cancel_placement()
    report['placement_feedback'] = building.feedback
    check('valid_camp_placement', building.valid_placement)
    check('build_workbench', building.confirm_placement())
    yield wait(lambda: building.building_count() == 1, 15)
    station = building.nearby_workbench()
    check('nearby_workbench', station != unreal.Guid())
    ui.open_page('crafting')
    check('crafting_page', str(ui.get_page()) == 'crafting')
    ui.open_page('hud')
    craft = unreal.HearthwardAgentGoal()
    for key, value in dict(intent='craft', item='arrows', quantity=1,
                           quantity_mode='batches', source_ref='camp', station=station).items():
        craft.set_editor_property(key, value)
    check('npc_craft_goal', ai.set_structured_goal(p, c, craft))
    check('npc_craft_confirm', ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda: c.get_phase() == unreal.HearthwardCompanionPhase.COMPLETED, 90)
    check('npc_craft_settlement', store.get_item_count('wood') == 1 and store.get_item_count('arrow') == 4)
    before = {x.save_id.to_string() for x in save.get_points()}
    check('save_built_camp', save.save_point(True))
    point = next(x.save_id for x in save.get_points() if x.save_id.to_string() not in before)
    # Destroy the world through ordinary level travel, then load via the title-screen action.
    unreal.GameplayStatics.open_level(w, '/Game/Hearthward/Bootstrap/L_Bootstrap')
    yield wait(lambda: unreal.GameplayStatics.get_current_level_name(world(), True) == 'L_Bootstrap')
    yield delay(2)
    menu = unreal.GameplayStatics.get_player_controller(world(), 0).get_hud().screen
    check('title_load', menu.execute_action('load:' + point.to_string()))
    yield wait(lambda: unreal.GameplayStatics.get_current_level_name(world(), True) == 'L_HearthwardWilds')
    yield delay(8)
    p = unreal.GameplayStatics.get_player_pawn(world(), 0)
    restored = p.get_component_by_class(unreal.HearthwardBuildingComponent)
    check('workbench_restored_after_travel', restored.building_count() == 1)
    store = sub(unreal.HearthwardStorageSubsystem)
    check('inventory_restored_after_travel', store.get_item_count('wood') == 1 and store.get_item_count('arrow') == 4)
    check('single_companion_after_travel', len(unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.HearthwardCompanionFixture)) == 1)

def legacy_natural(p, save, store):
    # Native automation emits a valid old-format file; install only into this command-line test pool.
    token = re.search(r'-HearthwardSaveTestPool=([0-9a-fA-F-]+)', unreal.SystemLibrary.get_command_line()).group(1).replace('-', '')
    pool_path = root / 'Saved/SaveGames/HearthwardPrototype' / ('test-' + token + '.hws')
    pool_path.write_bytes((out / 'legacy-natural.hws').read_bytes())
    save = sub(unreal.HearthwardSaveSubsystem)
    check('read_legacy_index', save.load_point_index())
    check('load_legacy_natural', save.load_point(save.get_points()[0].save_id))
    companion = unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.HearthwardCompanionFixture)[0]
    check('legacy_adds_companion', companion.can_communicate(p) and companion.source.get_item_count('wood') == 16)
    check('legacy_preserves_inventory', p.get_component_by_class(unreal.HearthwardInventoryComponent).get_item_count('wood') == 3
          and store.get_item_count('wood') == 5)
    check('legacy_enables_gameplay', p.get_component_by_class(unreal.HearthwardGameplayComponent).enabled)
    check('legacy_resaves_integrated', save.save_point(True))
    yield delay(.5)

def finish(error=None):
    if error:
        report['error'] = error
    report['ok'] = not error and bool(report['checks']) and all(report['checks'].values())
    (out / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    unreal.SystemLibrary.quit_editor()

flow = run()
pending = None
def tick(_dt):
    global pending
    try:
        if pending:
            pred, deadline = pending
            if not pred():
                if time.monotonic() > deadline:
                    raise TimeoutError('wait expired')
                return
        pending = next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
