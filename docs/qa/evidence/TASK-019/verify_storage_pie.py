"""Real interaction timer/containers/disk; two PIE worlds, isolated save pool."""
import json, os, re, time, traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task019'; out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'scope':'PROTOTYPE_ONLY'}
state={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(p,seconds=30): return p,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def command(text): unreal.SystemLibrary.execute_console_command(state['world'],text,state['pc'])
def capture(name): command('Shot showui filename="'+(out/(name+'.png')).as_posix()+'"')
def components():
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0); p=unreal.GameplayStatics.get_player_pawn(w,0)
    state.update(world=w,pc=pc,pawn=p)
    command('Hearthward.Companion.CreateTest')
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    state.update(companion=c,source=c.source,camp=c.camp,bag=p.get_component_by_class(unreal.HearthwardInventoryComponent),interaction=p.get_component_by_class(unreal.HearthwardInteractionComponent),action=p.get_component_by_class(unreal.HearthwardTimedActionComponent),save=sub(unreal.HearthwardSaveSubsystem,w),storage=sub(unreal.HearthwardStorageSubsystem,w))
def approach(actor):
    p=state['pawn']; p.character_movement.stop_movement_immediately()
    pos=actor.get_actor_location(); p.set_actor_location(unreal.Vector(pos.x-110,pos.y,pos.z),False,True)
    state['pc'].set_control_rotation(unreal.Rotator(0,0,0))
def counts(): return (state['source'].get_item_count('wood'),state['bag'].get_item_count('wood'),state['storage'].get_item_count('wood'),state['companion'].bag.get_item_count('wood'))
def run():
    assert re.search(r'-HearthwardSaveTestPool=[0-9a-fA-F-]+',unreal.SystemLibrary.get_command_line())
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor); yield delay(2)
    components(); s=state; hud=s['pc'].get_hud(); bag=s['bag']; storage=s['storage']; save=s['save']
    hud.toggle_storage_menu(); check('remote_cannot_open',not hud.is_storage_menu_open())
    approach(s['source'].get_owner()); yield delay(.3)
    hud.toggle_storage_menu(); check('resource_not_camp',not hud.is_storage_menu_open())
    approach(s['camp']); yield delay(.3)
    for item in ['wood','stone','ore','meat','arrow']: bag.try_add(item,4)
    check('explicit_save_enable',save.enable_prototype() and save.start_new_progress())
    hud.toggle_storage_menu(); menu=hud.get_storage_widget()
    check('camp_opens_and_pauses',menu and unreal.GameplayStatics.is_game_paused(s['world']))
    check('five_items_visible',all(x in menu.get_displayed_inventory() for x in ['木材','石材','矿石','肉','箭']))
    capture('storage-open'); yield delay(.5)
    for item in ['wood','stone','ore','meat','arrow']:
        menu.select_item(item); unreal.SystemLibrary.collect_garbage(); yield delay(.5)
        check(item+'_selection_survives_gc',hud.get_storage_widget()==menu)
        menu.set_quantity_text('3'); menu.deposit()
        check(item+'_deposit_conserves',bag.get_item_count(item)==1 and storage.get_item_count(item)==3)
        menu.set_quantity_text('2'); menu.withdraw()
        check(item+'_withdraw_conserves',bag.get_item_count(item)==3 and storage.get_item_count(item)==1)
    menu.select_item('wood')
    for invalid in ['0','-1','1.5','abc','2147483648','9999999999','','1e2']:
        menu.set_quantity_text(invalid); menu.deposit()
        check('invalid_'+repr(invalid),bag.get_item_count('wood')==3 and storage.get_item_count('wood')==1 and '整数' in menu.get_displayed_status())
    menu.set_quantity_text('4'); menu.deposit()
    check('insufficient_bag_atomic','库存不足' in menu.get_displayed_status() and bag.get_item_count('wood')==3)
    menu.set_quantity_text('2'); menu.withdraw()
    check('insufficient_storage_atomic','库存不足' in menu.get_displayed_status() and storage.get_item_count('wood')==1)
    bag.try_add('stone',86); menu.select_item('ore'); menu.set_quantity_text('1'); menu.withdraw()
    check('overweight_atomic','容量不足' in menu.get_displayed_status() and storage.get_item_count('ore')==1 and bag.get_item_count('ore')==3)
    capture('capacity-rejected'); yield delay(.3); bag.try_remove('stone',86)
    menu.select_item('wood'); menu.set_quantity_text('1')
    approach(s['source'].get_owner()); menu.deposit()
    check('distance_rechecked_each_click','失效' in menu.get_displayed_status() and bag.get_item_count('wood')==3)
    approach(s['camp']); storage.advance_timeline(); menu.deposit()
    check('stale_epoch_rejected','失效' in menu.get_displayed_status() and bag.get_item_count('wood')==3)
    hud.close_storage_menu(); check('close_restores_input_pause',not unreal.GameplayStatics.is_game_paused(s['world']) and not s['pc'].is_move_input_ignored() and not s['pc'].is_look_input_ignored())
    unreal.GameplayStatics.set_game_paused(s['world'],True); hud.toggle_storage_menu(); hud.close_storage_menu()
    check('external_pause_preserved',unreal.GameplayStatics.is_game_paused(s['world']))
    unreal.GameplayStatics.set_game_paused(s['world'],False)
    hud.toggle_storage_menu(); hud.toggle_inventory()
    check('inventory_exclusive',hud.is_inventory_open() and not hud.is_storage_menu_open())
    hud.toggle_inventory(); hud.toggle_storage_menu(); hud.toggle_save_menu()
    check('save_exclusive',hud.is_save_menu_open() and not hud.is_storage_menu_open())
    hud.close_save_menu(); hud.toggle_storage_menu(); hud.toggle_dialogue()
    check('dialogue_exclusive',not hud.is_dialogue_open() and hud.is_storage_menu_open())
    hud.close_storage_menu(); yield delay(.3)
    check('save_quantities',save.save_point(True)); saved=save.get_points()[-1].save_id
    hud.toggle_storage_menu(); menu=hud.get_storage_widget(); menu.select_item('wood'); menu.set_quantity_text('2'); menu.deposit()
    check('changed_before_load',bag.get_item_count('wood')==1 and storage.get_item_count('wood')==3)
    check('load_restores_quantities',save.load_point(saved) and bag.get_item_count('wood')==3 and storage.get_item_count('wood')==1)
    check('load_closes_old_session',not hud.is_storage_menu_open() and not unreal.GameplayStatics.is_game_paused(s['world']))
    hud.toggle_storage_menu(); fresh=hud.get_storage_widget(); menu.deposit()
    check('detached_widget_cannot_transfer',bag.get_item_count('wood')==3 and storage.get_item_count('wood')==1)
    fresh.select_item('wood'); fresh.set_quantity_text('1'); fresh.deposit()
    check('new_session_works',bag.get_item_count('wood')==2 and storage.get_item_count('wood')==2)
    s['camp'].destroy_actor(); fresh.withdraw()
    check('destroyed_camp_rejected','失效' in fresh.get_displayed_status() and bag.get_item_count('wood')==2)
    hud.close_storage_menu(); levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor()); yield delay(1)
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor); yield delay(2)
    components(); s=state; hud=s['pc'].get_hud(); save=s['save']; bag=s['bag']; storage=s['storage']
    check('second_pie_disk_restore',save.enable_prototype() and save.load_point(saved))
    check('all_five_restored',all(bag.get_item_count(i)==3 and storage.get_item_count(i)==1 for i in ['wood','stone','ore','meat','arrow']))
    approach(s['camp']); yield delay(.3); hud.toggle_storage_menu(); menu=hud.get_storage_widget()
    check('second_pie_menu_restored',menu and '仓储 1' in menu.get_displayed_inventory())
    menu.select_item('arrow'); menu.set_quantity_text('1'); menu.withdraw()
    check('second_pie_transfer',bag.get_item_count('arrow')==4 and storage.get_item_count('arrow')==0)
    capture('disk-restored'); yield delay(.5); hud.close_storage_menu()
    if os.environ.get('TASK019_INTERACTIVE')!='1':
        levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor())

def run_physical():
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor); yield delay(2)
    components(); s=state; hud=s['pc'].get_hud(); bag=s['bag']
    for item in ['wood','stone','ore','meat','arrow']: bag.try_add(item,4)
    approach(s['camp']); yield delay(.5); hud.toggle_storage_menu(); menu=hud.get_storage_widget()
    check('final_menu_opens',menu and unreal.GameplayStatics.is_game_paused(s['world']))
    menu.set_quantity_text('1'); menu.deposit()
    check('final_transfer',bag.get_item_count('wood')==3 and s['storage'].get_item_count('wood')==1)
    capture('final-menu'); yield delay(.5)
    hud.close_storage_menu(); check('final_input_restored',not s['pc'].is_move_input_ignored() and not unreal.GameplayStatics.is_game_paused(s['world']))

def observe(dt):
    if not levels.is_in_play_in_editor() or time.monotonic()-state.get('last',0)<.35:return
    state['last']=time.monotonic(); pos=state['pawn'].get_actor_location()
    data={'counts':counts(),'position':[pos.x,pos.y,pos.z],'interaction':str(state['interaction'].get_status()),'feedback':state['interaction'].get_completion_feedback(),'paused':unreal.GameplayStatics.is_game_paused(state['world']),'menu':state['pc'].get_hud().is_storage_menu_open()}
    (out/'interactive-state.json').write_text(json.dumps(data,ensure_ascii=False),encoding='utf-8')
    if (Path(unreal.Paths.project_dir())/'.agent-local/task019-record').exists():
        frames=out/'physical-frames'; frames.mkdir(exist_ok=True)
        i=state.get('frame',0);state['frame']=i+1
        command('Shot showui filename="'+(frames/('frame-%05d.png'%i)).as_posix()+'"')
        with (out/'physical-trace.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(dict(time=time.monotonic(),frame=i,**data),ensure_ascii=False)+'\n')
def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    report['passed']=not error and bool(report['checks']) and all(report['checks'].values())
    if error:report['error']=error
    (out/('physical-verification.json' if os.environ.get('TASK019_PHYSICAL_ONLY')=='1' else 'verification.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    if os.environ.get('TASK019_INTERACTIVE')=='1' and report['passed']:state['watch']=unreal.register_slate_post_tick_callback(observe)
    elif levels.is_in_play_in_editor():levels.editor_request_end_play()
flow=run_physical() if os.environ.get('TASK019_PHYSICAL_ONLY')=='1' else run();pending=None
def tick(dt):
    global pending
    try:
        if pending:
            p,end=pending
            if not p():
                if time.monotonic()>end:raise TimeoutError('interaction wait')
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
