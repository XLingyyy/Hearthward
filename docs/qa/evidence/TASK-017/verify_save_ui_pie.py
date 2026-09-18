"""TASK-017 real widget bindings + disk round trips, isolated pool, two PIE worlds.
Launch through UEClient with -HearthwardSaveTestPool=<fresh GUID>.
TASK017_INTERACTIVE=1 leaves the second PIE available for physical input review.
"""
import json, os, re, time, traceback, uuid
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task017'
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{},'scope':'PROTOTYPE_ONLY','provider':'real-save-subsystem'}
state={}

def check(name,value):
    report['checks'][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(predicate,seconds=30): return predicate,time.monotonic()+seconds
def delay(seconds):
    deadline=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=deadline)
def sub(cls,world): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
def command(world,pc,text): unreal.SystemLibrary.execute_console_command(world,text,pc)
def capture(name): command(state['world'],state['pc'],'Shot showui filename="'+(out/(name+'.png')).as_posix()+'"')
def same(a,b): return a.to_string()==b.to_string()
def point(save,id): return next(p for p in save.get_points() if same(p.save_id,id))

def run():
    assert re.search(r'-HearthwardSaveTestPool=[0-9a-fA-F-]+',unreal.SystemLibrary.get_command_line()),'isolated pool required'
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda:bool(unreal.GameplayStatics.get_player_pawn(world,0))); yield delay(1)
    pc=unreal.GameplayStatics.get_player_controller(world,0); pawn=unreal.GameplayStatics.get_player_pawn(world,0); hud=pc.get_hud()
    state.update(world=world,pc=pc,pawn=pawn)
    save=sub(unreal.HearthwardSaveSubsystem,world); clock=sub(unreal.HearthwardWorldClockSubsystem,world)
    storage=sub(unreal.HearthwardStorageSubsystem,world)
    hud.toggle_save_menu(); yield delay(.5); menu=hud.get_save_widget()
    check('open_pauses_and_claims_input',hud.is_save_menu_open() and unreal.GameplayStatics.is_game_paused(world) and pc.is_move_input_ignored() and pc.is_look_input_ignored() and pc.get_editor_property('show_mouse_cursor'))
    check('empty_list_and_unenabled_status',menu.get_displayed_point_count()==0 and '未启用' in menu.get_displayed_status() and '暂无' in menu.get_displayed_details())
    menu.call_method('EnableSaves'); check('missing_fixture_explained','夹具' in menu.get_displayed_status() and not save.is_prototype_enabled())
    capture('empty'); yield delay(.5)
    hud.close_save_menu(); command(world,pc,'Hearthward.Companion.CreateTest'); yield delay(.5)
    hud.toggle_save_menu(); menu=hud.get_save_widget(); menu.call_method('EnableSaves')
    check('explicit_enable_from_ui',save.is_prototype_enabled() and len(save.get_points())==0)
    menu.request_new_progress(); check('new_requires_confirmation',menu.has_pending_confirmation() and len(save.get_points())==0)
    menu.cancel_pending(); check('cancel_new_preserves_empty_pool',len(save.get_points())==0)
    menu.request_new_progress(); menu.confirm_pending(); yield delay(.3)
    check('confirmed_new_creates_single_point',len(save.get_points())==1 and menu.get_displayed_point_count()==1)
    initial=save.get_points()[0].save_id; campaign=save.get_campaign_id()
    check('metadata_exposed',all(x in menu.get_displayed_details() for x in ['所属进度','UTC','L_Bootstrap','阶段','自动','未锁定']))
    before=clock.get_snapshot().active_play_seconds; yield delay(1)
    check('menu_freezes_clock',abs(clock.get_snapshot().active_play_seconds-before)<.001)
    inv=pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    inv.try_add('wood',5); menu.save_manual(); saved=save.get_points()[-1].save_id
    menu.select_point(saved); menu.toggle_locked()
    check('save_and_lock_real_point',point(save,saved).manual and point(save,saved).locked and '已锁定' in menu.get_displayed_details())
    menu.toggle_locked(); check('unlock_real_point',not point(save,saved).locked)
    menu.select_point(initial); menu.request_delete(); menu.select_point(saved)
    check('confirmation_pins_original_selection',same(menu.get_selected_id(),initial) and '删除' in menu.get_confirmation_text())
    menu.save_manual(); check('pending_blocks_other_mutations',len(save.get_points())==2)
    menu.cancel_pending(); check('cancel_delete_preserves_point',len(save.get_points())==2)
    menu.select_point(saved); inv.try_add('wood',2); epoch=storage.get_timeline_epoch()
    menu.request_load(); menu.cancel_pending()
    check('cancel_load_preserves_world',inv.get_item_count('wood')==7 and same(epoch,storage.get_timeline_epoch()))
    menu.request_load(); menu.confirm_pending(); menu.confirm_pending(); yield delay(.4)
    check('ui_load_restores_inventory_and_epoch',inv.get_item_count('wood')==5 and not same(epoch,storage.get_timeline_epoch()))
    check('load_keeps_menu_pause_and_input',hud.is_save_menu_open() and unreal.GameplayStatics.is_game_paused(world) and pc.is_move_input_ignored())
    check('double_confirm_no_extra_point',len(save.get_points())==2 and not menu.has_pending_confirmation())
    menu.set_auto_minutes(1); check('interval_min',save.get_auto_minutes()==1)
    menu.set_auto_minutes(60); check('interval_max',save.get_auto_minutes()==60)
    menu.set_auto_minutes(61); check('interval_invalid_rejected',save.get_auto_minutes()==60 and '1—60' in menu.get_displayed_status())
    menu.set_auto_minutes(10)
    n=len(save.get_points())
    for field,text in [('combat','战斗'),('either_downed','倒地'),('pursued','追击'),('drowning','溺水'),('falling','坠落'),('companion_danger','弟弟')]:
        safety=unreal.HearthwardSaveSafety(); setattr(safety,field,True); save.set_prototype_safety(safety); menu.save_manual()
        check('danger_reason_'+field,len(save.get_points())==n and text in menu.get_displayed_status())
    capture('danger'); yield delay(.5)
    safety=unreal.HearthwardSaveSafety(); safety.severe_hunger=True; save.set_prototype_safety(safety); menu.save_manual()
    check('hunger_allowed_and_warned',len(save.get_points())==n+1 and '严重饥饿' in menu.get_displayed_status())
    save.set_prototype_safety(unreal.HearthwardSaveSafety())
    while len(save.get_points())<50: assert save.save_point(True),save.get_status()
    save.set_point_locked(initial,True); menu.refresh_points(); menu.select_point(saved)
    check('all_50_real_nodes_listed',menu.get_displayed_point_count()==50)
    menu.save_manual(); check('full_protected_failure_visible',len(save.get_points())==50 and '无可用' in menu.get_displayed_status())
    menu.scroll_to_end(); yield delay(.5); capture('full-list'); yield delay(.5)
    menu.request_new_progress(); menu.confirm_pending()
    check('full_pool_new_rejected',same(campaign,save.get_campaign_id()) and len(save.get_points())==50 and '无可用' in menu.get_displayed_status())
    menu.select_point(save.get_points()[-1].save_id); menu.request_delete(); menu.confirm_pending()
    check('confirmed_delete_frees_real_capacity',len(save.get_points())==49 and menu.get_displayed_point_count()==49)
    menu.request_new_progress(); menu.confirm_pending()
    check('new_progress_resets_world',len(save.get_points())==50 and not same(campaign,save.get_campaign_id()) and inv.get_item_count('wood')==0)
    menu.select_point(saved); menu.request_load(); menu.confirm_pending()
    check('cross_progress_load',same(campaign,save.get_campaign_id()) and inv.get_item_count('wood')==5)
    menu.request_quit(); check('quit_warning_without_hidden_save','未保存' in menu.get_confirmation_text() and '不会额外保存' in menu.get_confirmation_text() and len(save.get_points())==50)
    capture('quit-confirmation'); yield delay(.5); menu.cancel_pending()
    check('cancel_quit_preserves_session',levels.is_in_play_in_editor() and len(save.get_points())==50)
    hud.close_save_menu(); yield delay(.5)
    check('close_releases_owned_pause_and_input',not unreal.GameplayStatics.is_game_paused(world) and not pc.is_move_input_ignored() and not pc.is_look_input_ignored() and not pc.get_editor_property('show_mouse_cursor'))
    unreal.GameplayStatics.set_game_paused(world,True); hud.toggle_save_menu(); hud.close_save_menu()
    check('external_pause_preserved',unreal.GameplayStatics.is_game_paused(world))
    unreal.GameplayStatics.set_game_paused(world,False)
    hud.toggle_inventory(); hud.toggle_save_menu()
    check('inventory_to_save_exclusive',hud.is_save_menu_open() and not hud.is_inventory_open() and unreal.GameplayStatics.is_game_paused(world))
    hud.toggle_inventory(); check('save_to_inventory_exclusive',not hud.is_save_menu_open() and hud.is_inventory_open() and unreal.GameplayStatics.is_game_paused(world))
    hud.toggle_inventory(); hud.toggle_dialogue(); yield delay(.3)
    check('dialogue_before_switch',hud.is_dialogue_open())
    hud.toggle_save_menu(); check('dialogue_to_save_exclusive',not hud.is_dialogue_open() and hud.is_save_menu_open())
    hud.toggle_dialogue(); check('dialogue_cannot_overlay_save',not hud.is_dialogue_open())
    hud.close_save_menu(); check('switching_does_not_leak_input_counter',not pc.is_move_input_ignored() and not pc.is_look_input_ignored())
    # Corruption is injected only into this session's isolated test pool.
    match=re.search(r'-HearthwardSaveTestPool=([0-9a-fA-F-]+)',unreal.SystemLibrary.get_command_line())
    pool=Path(unreal.Paths.project_saved_dir())/'SaveGames/HearthwardPrototype'/('test-'+uuid.UUID(match.group(1)).hex+'.hws')
    original=pool.read_bytes(); corrupt=bytearray(original); corrupt[-1]^=1
    hud.toggle_save_menu(); menu=hud.get_save_widget(); menu.select_point(saved)
    try:
        pool.write_bytes(corrupt); menu.request_load(); menu.confirm_pending()
        check('corrupt_pool_error_visible','校验' in menu.get_displayed_status() and inv.get_item_count('wood')==5 and menu.get_displayed_point_count()==0)
    finally: pool.write_bytes(original)
    # Reopen/re-enable in the next world must read actual on-disk metadata.
    levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor()); yield delay(1)
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda:bool(unreal.GameplayStatics.get_player_pawn(world,0))); yield delay(1)
    pc=unreal.GameplayStatics.get_player_controller(world,0); pawn=unreal.GameplayStatics.get_player_pawn(world,0); hud=pc.get_hud()
    state.update(world=world,pc=pc,pawn=pawn)
    check('second_pie_clean_input',not hud.is_save_menu_open() and not pc.is_move_input_ignored() and not pc.get_editor_property('show_mouse_cursor'))
    command(world,pc,'Hearthward.Companion.CreateTest'); hud.toggle_save_menu(); menu=hud.get_save_widget(); menu.call_method('EnableSaves')
    check('second_pie_lists_disk_points',menu.get_displayed_point_count()==50)
    menu.select_point(saved); menu.request_load(); menu.confirm_pending()
    check('second_pie_ui_restore',pawn.get_component_by_class(unreal.HearthwardInventoryComponent).get_item_count('wood')==5 and '已恢复' in menu.get_displayed_status())
    capture('restored'); yield delay(.5)
    if os.environ.get('TASK017_INTERACTIVE')!='1':
        levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor())

def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    report['passed']=not error and bool(report['checks']) and all(report['checks'].values())
    if error: report['error']=error
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    if os.environ.get('TASK017_INTERACTIVE')=='1' and report['passed']:
        state['watch']=unreal.register_slate_post_tick_callback(observe)
    elif levels.is_in_play_in_editor(): levels.editor_request_end_play()

def observe(dt):
    if not levels.is_in_play_in_editor(): return
    if time.monotonic()-state.get('last',0)<.4: return
    state['last']=time.monotonic()
    hud=state['pc'].get_hud(); menu=hud.get_save_widget(); pos=state['pawn'].get_actor_location()
    data={'menu_open':hud.is_save_menu_open(),'inventory_open':hud.is_inventory_open(),'dialogue_open':hud.is_dialogue_open(),
          'paused':unreal.GameplayStatics.is_game_paused(state['world']),'move_ignored':state['pc'].is_move_input_ignored(),
          'point_count':menu.get_displayed_point_count() if menu else None,'selected':menu.get_selected_id().to_string() if menu else None,
          'confirmation':menu.get_confirmation_text() if menu else '', 'status':menu.get_displayed_status() if menu else '', 'position':[pos.x,pos.y,pos.z]}
    (out/'interactive-state.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    if (Path(unreal.Paths.project_dir())/'.agent-local/task017-record').exists():
        frames=out/'physical-frames'; frames.mkdir(exist_ok=True)
        index=state.get('frame',0); state['frame']=index+1
        command(state['world'],state['pc'],'Shot showui filename="'+(frames/('frame-%05d.png'%index)).as_posix()+'"')
        with (out/'physical-trace.jsonl').open('a',encoding='utf-8') as stream:
            stream.write(json.dumps(dict(time=time.monotonic(),frame=index,**data),ensure_ascii=False)+'\n')

flow=run(); pending=None
def tick(dt):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline: raise TimeoutError('UI state wait')
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
