"""Native nine-page renderer and real gameplay transactions in an isolated save pool."""
import json,time,traceback,re
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'; out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'scope':'TASK-020 native UI and playable greybox; not complete world art'}
state={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(p,seconds=45):return p,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end)
def sub(cls,w):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def approach(actor):
    p=state['pawn'];pos=actor.get_actor_location();p.character_movement.stop_movement_immediately()
    p.set_actor_location(unreal.Vector(pos.x-100,pos.y,pos.z),False,True)
def run():
    assert re.search(r'-HearthwardSaveTestPool=[0-9a-fA-F-]+',unreal.SystemLibrary.get_command_line())
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(3)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);p=unreal.GameplayStatics.get_player_pawn(w,0)
    hud=pc.get_hud();ui=hud.screen;g=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    bag=p.get_component_by_class(unreal.HearthwardInventoryComponent);store=sub(unreal.HearthwardStorageSubsystem,w);save=sub(unreal.HearthwardSaveSubsystem,w)
    state.update(world=w,pc=pc,pawn=p,ui=ui)
    ui.open_page('title');yield delay(.5)
    check('title_native_capture',ui.capture_ui('title',1672,941))
    check('new_campaign',ui.execute_action('new'))
    yield delay(.5)
    check('real_player_status',g.enabled and g.health>0 and bag.get_item_count('axe')==1)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    ui.open_page('inventory');ui.execute_action('item:axe');check('equip_real_owned_item',ui.execute_action('use') and str(g.equipment.get('weapon'))=='axe')
    for item in ('hood','armor','gloves','belt','boots','shield','bow','quiver','amulet'):check('equip_'+item,g.equip(item))
    ui.open_page('inventory')
    check('inventory_native_capture',ui.capture_ui('inventory',1672,941))
    h=g.hunger;yield delay(.5);check('menu_freezes_hunger',g.hunger==h)
    check('skill_parent_rejected',not g.learn('vigor'))
    check('learn_root',g.learn('strong'))
    check('learn_child',g.learn('vigor'))
    check('points_spent',g.skill_points()==0 and g.max_health()>100)
    check('overspend_rejected',not g.learn('trail'))
    ui.open_page('skills');check('skills_native_capture',ui.capture_ui('skills',1672,941))
    g.reset_skills();check('free_respec',g.skill_points()==2 and g.max_health()==100)
    bag.try_add('wood',4);check('quest_real_inventory',g.quest_progress('ember')==4)
    check('claim_once',g.claim('ember') and not g.claim('ember'))
    approach(c.camp);check('storage_access',ui.execute_action('page:storage'))
    ui.execute_action('deposit:wood');ui.execute_action('quantity:1');check('transfer_real_items',ui.execute_action('transfer') and bag.get_item_count('wood')==2 and store.get_item_count('wood')==2)
    check('storage_native_capture',ui.capture_ui('storage',1672,941))
    ui.execute_action('withdraw:wood');check('withdraw_real_items',ui.execute_action('transfer') and bag.get_item_count('wood')==3 and store.get_item_count('wood')==1)
    store.advance_timeline();check('stale_storage_denied',not ui.execute_action('transfer'))
    ui.open_page('map');check('unactivated_travel_denied',not g.travel('watch'))
    ui.open_page('hud');approach(c.camp);yield delay(.4)
    meat_before=bag.get_item_count('roast');check('food_consumes_actual_stack',g.use_item('roast') and bag.get_item_count('roast')==meat_before-1 and g.hunger==100)
    camp_pos=c.camp.get_actor_location()
    p.set_actor_location(unreal.Vector(camp_pos.x+1400,camp_pos.y+1200,camp_pos.z),False,True);yield delay(.5)
    check('approach_discovers_without_activation','watch' in [str(x) for x in g.discovered] and 'watch' not in [str(x) for x in g.activated])
    check('interaction_activates',g.activate_nearby())
    ui.open_page('map')
    clock=sub(unreal.HearthwardWorldClockSubsystem,w).get_snapshot().active_play_seconds
    check('activated_travel_from_station',g.travel('camp'))
    check('watch_destination_collision_clear',g.travel('watch') and g.travel('camp'))
    check('travel_does_not_advance_clock',sub(unreal.HearthwardWorldClockSubsystem,w).get_snapshot().active_play_seconds==clock)
    check('map_native_capture',ui.capture_ui('map',1672,941))
    ui.open_page('journal');check('journal_native_capture',ui.capture_ui('journal',1672,941))
    ui.open_page('pause');check('pause_native_capture',ui.capture_ui('pause',1672,941))
    ui.open_page('dialogue');check('dialogue_native_capture',ui.capture_ui('dialogue',1672,941))
    ui.open_page('hud');yield delay(.5);check('hud_native_capture',ui.capture_ui('hud',1672,941))
    check('unpause_restored',not unreal.GameplayStatics.is_game_paused(w))
    ui.open_page('inventory');check('small_resolution_capture',ui.capture_ui('inventory-1280',1280,720))
    unreal.SystemLibrary.collect_garbage();yield delay(.5)
    check('textures_survive_gc',ui.capture_ui('inventory-after-gc',1672,941))
    g.set_waypoint(unreal.Vector(camp_pos.x+500,camp_pos.y+300,camp_pos.z))
    before=g.experience;check('save_extended_state',save.save_point(True))
    point=save.get_points()[-1]
    g.learn('strong');bag.try_remove('axe',1);check('removing_equipped_item_unequips',not g.equipment.get('weapon'))
    check('load_extended_state',save.load_point(point.save_id))
    check('equipment_and_inventory_restored',str(g.equipment.get('weapon'))=='axe' and bag.get_item_count('axe')==1)
    check('quest_reward_restored',g.experience==before and not g.claim('ember'))
    check('map_state_restored','watch' in [str(x) for x in g.activated])
    check('custom_waypoint_restored',g.has_waypoint and abs(g.waypoint.x-camp_pos.x-500)<1)
    ui.open_page('hud')
    p.set_actor_location(unreal.Vector(camp_pos.x+1000,camp_pos.y-500,camp_pos.z),False,True)
    yield delay(.4)
    check('enemy_causes_damage',g.health<g.max_health() and g.in_combat())
    check('combat_blocks_save',not save.save_point(True))
    medicine=bag.get_item_count('medicine');hurt=g.health
    check('medicine_heals_and_consumes',g.use_item('medicine') and g.health>hurt and bag.get_item_count('medicine')==medicine-1)
    health_before=dict(g.opponents);wear_before=dict(g.durability);stamina_before=g.stamina
    check('real_attack_hits',g.attack())
    check('attack_spends_stamina',g.stamina<stamina_before)
    check('attack_wears_weapon',g.durability.get('axe')<wear_before.get(unreal.Name('axe')))
    check('attack_changes_opponent_health',sum(g.opponents.values())<sum(health_before.values()))
    check('attack_cooldown',not g.attack())
    check('heavy_attack_requires_skill',not g.heavy_attack())
    g.learn('strong');yield delay(.7)
    target_before=sum(g.opponents.values());check('heavy_attack_executes',g.heavy_attack())
    check('heavy_attack_real_multiplier',abs(target_before-sum(g.opponents.values())-30)<.01)
    for _ in range(6):
        yield delay(.7)
        g.attack()
    check('enemy_defeat_grants_xp',g.experience>before and any(v==0 for v in g.opponents.values()))
    arrows=bag.get_item_count('arrow');yield delay(.7)
    check('ranged_attack_consumes_arrow',g.shoot() and bag.get_item_count('arrow')==arrows-1)
    yield delay(.7);pots=bag.get_item_count('firepot');enemy_health=sum(g.opponents.values())
    check('throw_consumes_and_damages',g.throw_item('firepot') and bag.get_item_count('firepot')==pots-1 and abs(enemy_health-sum(g.opponents.values())-30)<.01)
    approach(c.camp);yield delay(3.3)
    check('combat_ends_after_retreat',not g.in_combat())
    bag.try_add('wood',2);bag.try_add('ore',1);wood_before=bag.get_item_count('wood');ore_before=bag.get_item_count('ore')
    check('repair_consumes_materials',g.repair('axe') and g.durability.get('axe')==100 and bag.get_item_count('wood')==wood_before-2 and bag.get_item_count('ore')==ore_before-1)
    check('full_durability_repair_rejected',not g.repair('axe'))
    check('save_enemy_and_durability',save.save_point(True))
    combat_point=save.get_points()[-1];opponents_saved=dict(g.opponents);wear_saved=dict(g.durability)
    resume={'pool':re.search(r'-HearthwardSaveTestPool=([0-9a-fA-F-]+)',unreal.SystemLibrary.get_command_line()).group(1),'point':combat_point.save_id.to_string(),'experience':g.experience,'opponents':{str(k):v for k,v in g.opponents.items()},'durability':{str(k):v for k,v in g.durability.items()}}
    (out/'resume.json').write_text(json.dumps(resume),encoding='utf-8')
    g.apply_damage(10)
    check('load_enemy_and_durability',save.load_point(combat_point.save_id) and dict(g.opponents)==opponents_saved and dict(g.durability)==wear_saved)
    check('new_progress_resets_gameplay',ui.execute_action('new') and g.experience==0 and not g.equipment and all(v==100 for v in g.opponents.values()))
    ticket=c.request(p,'collect four wood')
    accepted=c.submit(p,ticket,'wood',4,['collect','return','deposit'])
    yield delay(15)
    report['companion_diagnostic']={'phase':str(c.get_phase()),'block':c.block_reason,'delivered':c.get_delivered(),'requested':c.get_requested(),'camp':str(c.camp.get_actor_location()),'source':str(c.source.get_owner().get_actor_location()),'companion':str(c.get_actor_location())}
    check('companion_real_delivery_from_default_start',c.get_delivered()==4 and store.get_item_count('wood')==4)
    p.set_actor_location(unreal.Vector(camp_pos.x+450,camp_pos.y,camp_pos.z),False,True)
    companion_start=c.get_actor_location()
    check('companion_follow_order',g.order_companion('follow'))
    yield delay(1)
    check('companion_follows_with_real_motion',c.get_actor_location().distance(companion_start)>50)
    check('companion_wait_order',g.order_companion('wait'))
    companion_wait=c.get_actor_location();yield delay(.5)
    check('companion_wait_stops_motion',c.get_actor_location().distance(companion_wait)<1)
    g.set_waypoint(unreal.Vector(camp_pos.x+500,camp_pos.y+300,camp_pos.z))
    check('companion_follow_can_save',g.order_companion('follow') and save.save_point(True))
    follow_point=save.get_points()[-1]
    g.order_companion('wait');ui.execute_action('clearWaypoint')
    check('companion_order_and_marker_restore',save.load_point(follow_point.save_id) and str(g.companion_order)=='follow' and g.has_waypoint)
    p.set_actor_location(unreal.Vector(camp_pos.x+900,camp_pos.y-600,camp_pos.z),False,True)
    c.set_actor_location(unreal.Vector(camp_pos.x+850,camp_pos.y-600,camp_pos.z),False,True)
    damage_before=sum(g.opponents.values())
    check('companion_attack_order',g.order_companion('attack'))
    yield delay(1.5)
    report['companion_attack_initial']={'phase':str(c.get_phase()),'order':str(g.companion_order),'blocked':c.block_reason,'position':str(c.get_actor_location()),'health':g.health,'enabled':g.enabled,'paused':unreal.GameplayStatics.is_game_paused(w),'enemy_health':sum(g.opponents.values())}
    c.set_actor_location(unreal.Vector(camp_pos.x+1000,camp_pos.y-450,camp_pos.z),False,True)
    p.set_actor_location(unreal.Vector(camp_pos.x+850,camp_pos.y-450,camp_pos.z),False,True)
    yield delay(1.5)
    check('companion_attack_changes_real_enemy_health',sum(g.opponents.values())<damage_before)
    g.order_companion('wait')
    ui.open_page('title');report['passed']=all(report['checks'].values())
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    yield delay(.2)
iterator=run();pending=None
def tick(delta):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError('PIE wait')
                return
        pending=next(iterator)
    except StopIteration:
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();report['passed']=False
        (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.log_error(report['error']);unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
