"""Capture nine native pages after a layout-only change, using real campaign state."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'scope':'native UI capture; HUD capture excludes the 3D scene'}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>end
def run():
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    ui=unreal.GameplayStatics.get_player_controller(w,0).get_hud().screen
    g=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    check('title_capture',ui.capture_ui('title',1672,941))
    check('new_campaign',ui.execute_action('new'))
    for item in ('axe','hood','armor','gloves','belt','boots','shield','bow','quiver','amulet'):check('equip_'+item,g.equip(item))
    bag.try_add('wood',4)
    for page in ('inventory','storage','pause','dialogue','map','skills','journal','hud'):
        if page=='storage':
            c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
            pos=c.camp.get_actor_location();p.character_movement.stop_movement_immediately()
            p.set_actor_location(unreal.Vector(pos.x-100,pos.y,pos.z),False,True)
            check('storage_access',ui.execute_action('page:storage'))
        else:ui.open_page(page)
        yield delay(.3)
        if page=='inventory':ui.execute_action('item:axe')
        if page=='storage':
            ui.execute_action('deposit:wood');ui.execute_action('quantity:1');check('storage_transaction',ui.execute_action('transfer'))
        check(page+'_capture',ui.capture_ui(page,1672,941))
    ui.open_page('inventory');check('small_resolution',ui.capture_ui('inventory-1280',1280,720))
    ui.open_page('journal')
    for category in ('side','world','people','factions','collection'):
        check('open_journal_'+category,ui.execute_action('category:'+category))
        yield delay(.3);check('capture_journal_'+category,ui.capture_ui('journal-'+category,1672,941))
    ui.open_page('hud')
    report['passed']=True
    (out/'visual-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
iterator=run();pending=None;deadline=time.monotonic()+120
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('visual capture')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        (out/'visual-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
