"""Verify migration from an isolated copy of a genuine TASK-019 save and journal state."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'source':'TASK-019 test-D12D07D938EC4C54B2B0A26B63754313.hws, copied to a new test pool'}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>end
def run():
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    p=unreal.GameplayStatics.get_player_pawn(w,0);ui=unreal.GameplayStatics.get_player_controller(w,0).get_hud().screen
    g=p.get_component_by_class(unreal.HearthwardGameplayComponent);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer()==w)
    store=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer()==w)
    unreal.SystemLibrary.execute_console_command(w,'Hearthward.Companion.CreateTest')
    check('prepare_old_pool',save.enable_prototype())
    point=save.get_points()[0]
    check('legacy_load_without_adventure',save.load_point(point.save_id) and not g.enabled)
    catalog=json.loads((Path(unreal.Paths.project_dir())/'Resources/Data/gameplay.json').read_text(encoding='utf-8'))
    old_inventory={row['id']:bag.get_item_count(row['id']) for row in catalog['items']}
    old_storage={row['id']:store.get_item_count(row['id']) for row in catalog['items']}
    old_knowledge=save.get_knowledge()
    check('old_save_predates_equipment',old_inventory['axe']==0 and old_storage['axe']==0)
    g.enable_adventure()
    check('old_load',save.load_point(point.save_id))
    check('adventure_enabled',g.enabled and g.health==100)
    check('old_inventory_preserved',all(bag.get_item_count(k)==v for k,v in old_inventory.items()))
    check('old_storage_preserved',all(store.get_item_count(k)>=v for k,v in old_storage.items()))
    check('initial_equipment_in_storage',store.get_item_count('axe')==1+old_storage['axe'])
    check('old_knowledge_preserved',save.get_knowledge()==old_knowledge)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    pos=c.camp.get_actor_location();p.set_actor_location(unreal.Vector(pos.x-100,pos.y,pos.z),False,True)
    check('open_storage',ui.execute_action('page:storage'))
    # The old fixture may have a full bag; make room through an actual deposit.
    if bag.get_item_count('wood'):
        ui.execute_action('deposit:wood');ui.execute_action('quantity:'+str(bag.get_item_count('wood')-1));check('deposit_old_wood',ui.execute_action('transfer'))
    ui.execute_action('withdraw:axe');check('withdraw_migrated_equipment',ui.execute_action('transfer'))
    check('collection_records_acquisition',g.events.get('collected:axe')==1)
    ui.execute_action('deposit:axe');check('deposit_collected_item',ui.execute_action('transfer'))
    check('collection_retains_history',g.events.get('collected:axe')==1 and bag.get_item_count('axe')==0)
    check('save_migrated_state',save.save_point(True));migrated=save.get_points()[-1];axes=store.get_item_count('axe')
    check('reload_migrated_state',save.load_point(migrated.save_id))
    check('no_duplicate_migration_kit',store.get_item_count('axe')==axes)
    check('collection_persisted',g.events.get('collected:axe')==1)
    ui.open_page('journal')
    for category in ('main','side','world','people','factions','collection'):
        check('journal_'+category,ui.execute_action('category:'+category))
        yield delay(.3);check('capture_'+category,ui.capture_ui('journal-'+category,1672,941))
    check('discovered_camp_codex','camp' in [str(x) for x in g.discovered])
    report['passed']=True
    (out/'extension-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
iterator=run();pending=None;deadline=time.monotonic()+120
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('extension verification')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        (out/'extension-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
