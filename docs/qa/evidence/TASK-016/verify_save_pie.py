"""TASK-016: real USaveGame files, two PIE worlds, actual model request cancellation.
Uses a unique -HearthwardSaveTestPool GUID; never overwrites the user's prototype pool.
"""
import json, time, traceback, re, uuid
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir())/'Saved/Task016'
out.mkdir(parents=True,exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {'passed':False,'checks':{},'model':'Qwen3.5-4B Q4_K_M / llama.cpp b10964 / Vulkan 32','scope':'PROTOTYPE_ONLY'}
P=unreal.HearthwardCompanionPhase
R=unreal.HearthwardInventoryResult

def check(name,condition):
    report['checks'][name]=bool(condition)
    if not condition: raise AssertionError(name)
def wait(predicate,seconds=120): return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda: time.monotonic()>=end)
def sub(cls,world): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
def command(world,pc,text): unreal.SystemLibrary.execute_console_command(world,text,pc)
def latest(save): return save.get_points()[-1].save_id
def same(a,b): return a.to_string()==b.to_string()

def run():
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world,0)))
    yield delay(1)
    pawn=unreal.GameplayStatics.get_player_pawn(world,0); pc=unreal.GameplayStatics.get_player_controller(world,0)
    hud=pc.get_hud()
    save=sub(unreal.HearthwardSaveSubsystem,world)
    ai=sub(unreal.HearthwardLocalAISubsystem,world)
    storage=sub(unreal.HearthwardStorageSubsystem,world)
    clock=sub(unreal.HearthwardWorldClockSubsystem,world)
    check('requires_explicit_fixture',not save.enable_prototype())
    command(world,pc,'Hearthward.Companion.CreateTest')
    comp=unreal.GameplayStatics.get_all_actors_with_tag(world,'Hearthward.Companion.PROTOTYPE_ONLY')[0]
    pawn.set_actor_location(unreal.Vector(-450,-200,90),False,True)
    comp.set_actor_location(unreal.Vector(0,400,90),False,True)
    comp.camp.set_actor_location(unreal.Vector(0,400,90),False,True)
    comp.source.get_owner().set_actor_location(unreal.Vector(600,400,90),False,True)
    yield delay(.5)
    check('explicit_enable',save.enable_prototype())
    check('initial_node_created',save.start_new_progress() and len(save.get_points())==1)
    initial=latest(save); campaign=save.get_campaign_id()
    check('default_auto_interval',save.get_auto_minutes()==10)
    check('interval_bounds',not save.set_auto_minutes(0) and not save.set_auto_minutes(61))
    inv=pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    command(world,pc,'Hearthward.Storage.CreateTestAccess')
    access=unreal.GameplayStatics.get_all_actors_with_tag(world,'Hearthward.Storage.TestAccess')[0].get_component_by_class(unreal.HearthwardStorageAccessComponent)
    inv.try_add('wood',10)
    access.transfer(inv,True,'wood',3,unreal.GuidLibrary.new_guid(),storage.get_timeline_epoch())
    check('real_history_request',ai.submit_player_text(pawn,comp,'记住，我称这片营地为青石营。'))
    yield wait(lambda: not ai.is_busy())
    check('raw_exchange_retained','青石营' in save.get_knowledge() and bool(ai.get_npc_line()))
    ticket=comp.request(pawn,'PROTOTYPE_ONLY collect four')
    check('real_task_accepted',comp.submit(pawn,ticket,'wood',4,['collect','return','deposit'])==unreal.HearthwardProposalResult.ACCEPTED)
    yield wait(lambda: comp.get_phase()==P.GATHERING,30)
    yield delay(1)
    check('save_inflight_task',save.save_point(True))
    middle=latest(save); saved_time=clock.get_snapshot().active_play_seconds
    saved_elapsed=comp.action.get_elapsed_seconds(); saved_pos=pawn.get_actor_location()
    saved_knowledge=save.get_knowledge(); old_epoch=storage.get_timeline_epoch()
    yield wait(lambda: comp.get_phase()==P.COMPLETED,30)
    check('future_delivery_real',storage.get_item_count('wood')==7 and comp.source.get_item_count('wood')==12)
    check('future_exchange_request',ai.submit_player_text(pawn,comp,'记住未来暗号为赤狐。'))
    yield wait(lambda: not ai.is_busy())
    check('future_knowledge_exists','赤狐' in save.get_knowledge())
    inv.try_add('stone',2)
    pawn.set_actor_location(saved_pos+unreal.Vector(100,0,0),False,True)
    hud.toggle_dialogue(); hud.get_dialogue_widget().set_draft('future UI draft')
    stale=comp.request(pawn,'late structured response')
    check('restore_middle',save.load_point(middle))
    check('atomic_inventory_world_restore',inv.get_item_count('wood')==7 and inv.get_item_count('stone')==0 and storage.get_item_count('wood')==3 and comp.source.get_item_count('wood')==16 and comp.bag.get_weight()==0)
    check('time_and_position_restore',abs(clock.get_snapshot().active_play_seconds-saved_time)<.15 and sum(abs(getattr(pawn.get_actor_location(),a)-getattr(saved_pos,a)) for a in ('x','y','z'))<1)
    check('knowledge_rollback',save.get_knowledge()==saved_knowledge and '赤狐' not in save.get_knowledge())
    check('timer_and_task_resume',comp.get_phase()==P.GATHERING and abs(comp.action.get_elapsed_seconds()-saved_elapsed)<.15 and comp.get_requested()==4)
    check('new_runtime_epoch',not same(old_epoch,storage.get_timeline_epoch()))
    check('stale_proposal_rejected',comp.submit(pawn,stale,'wood',4,['collect','return','deposit'])==unreal.HearthwardProposalResult.STALE)
    check('stale_inventory_rejected',access.transfer(inv,True,'wood',1,unreal.GuidLibrary.new_guid(),old_epoch).result==R.STALE_TIMELINE)
    check('ui_and_ai_future_cleared',not hud.is_dialogue_open() and not pc.is_move_input_ignored() and not ai.get_npc_line() and not ai.get_last_filtered_context() and not ai.get_last_structured_result())
    yield wait(lambda: comp.get_phase()==P.COMPLETED,30)
    check('resume_no_duplicate_delivery',storage.get_item_count('wood')==7 and comp.get_delivered()==4 and comp.source.get_item_count('wood')==12)
    check('completed_snapshot_saved',save.save_point(True)); completed=latest(save)
    check('start_real_pending_model',ai.submit_player_text(pawn,comp,'帮我收集两份木材运回营地。'))
    yield wait(lambda: '思考' in ai.get_status(),30)
    check('load_during_real_http',ai.is_busy() and save.load_point(completed))
    yield delay(8)
    check('late_http_does_not_execute_or_remember',not ai.is_busy() and not ai.get_npc_line() and comp.get_phase()==P.COMPLETED and storage.get_item_count('wood')==7 and save.get_knowledge()==saved_knowledge)
    check('new_query_after_load',ai.submit_player_text(pawn,comp,'你好。'))
    yield wait(lambda: not ai.is_busy())
    check('rag_uses_restored_boundary','青石营' in ai.get_last_filtered_context() and '赤狐' not in ai.get_last_filtered_context() and bool(ai.get_npc_line()))
    # Restore again so restart comparisons have an exact known knowledge boundary.
    check('repeat_load_no_reward_replay',save.load_point(completed) and storage.get_item_count('wood')==7)
    check('start_another_progress',save.start_new_progress())
    other_campaign=save.get_campaign_id()
    check('progress_isolation',not same(campaign,other_campaign) and not save.get_knowledge() and inv.get_weight()==0 and storage.get_weight()==0 and comp.get_phase()==P.IDLE)
    check('return_to_first_progress',save.load_point(completed) and same(campaign,save.get_campaign_id()) and save.get_knowledge()==saved_knowledge)
    # A real 1-minute active-play timer; pause must not advance it.
    check('configure_auto_minute',save.set_auto_minutes(1))
    safety=unreal.HearthwardSaveSafety(); safety.combat=True; save.set_prototype_safety(safety)
    before_count=len(save.get_points()); before=clock.get_snapshot().active_play_seconds
    check('manual_danger_refused',not save.save_point(True) and len(save.get_points())==before_count)
    hud.toggle_inventory(); yield delay(1)
    check('pause_freezes_save_clock',abs(clock.get_snapshot().active_play_seconds-before)<.1)
    hud.toggle_inventory()
    yield wait(lambda: clock.get_snapshot().active_play_seconds>=before+61,85)
    check('due_auto_deferred',len(save.get_points())==before_count and '延后' in save.get_status())
    safety.combat=False; safety.severe_hunger=True; save.set_prototype_safety(safety)
    yield wait(lambda: len(save.get_points())==before_count+1,5)
    check('safe_resume_and_hunger_warning','严重饥饿' in save.get_status())
    # Fill the same real disk pool using synchronous snapshots, then protect every slot.
    hud.toggle_inventory()
    while len(save.get_points())<50:
        assert save.save_point(True),save.get_status()
    check('real_disk_pool_reaches_50',len(save.get_points())==50)
    for point in save.get_points():
        if not point.manual: assert save.set_point_locked(point.save_id,True)
    ids=[p.save_id.to_string() for p in save.get_points()]
    check('full_protected_auto_refused',not save.save_point(False) and ids==[p.save_id.to_string() for p in save.get_points()])
    check('full_protected_new_refused',not save.start_new_progress() and len(save.get_points())==50 and same(campaign,save.get_campaign_id()))
    check('unlock_old_auto',save.set_point_locked(initial,False))
    check('rotation_replaces_only_unlocked_auto',save.save_point(False) and len(save.get_points())==50 and initial.to_string() not in [p.save_id.to_string() for p in save.get_points()])
    check('new_can_use_replaceable_auto_capacity',save.start_new_progress() and len(save.get_points())==50)
    for point in save.get_points():
        if not point.manual: assert save.set_point_locked(point.save_id,True)
    check('new_again_requires_free_when_all_protected',not save.start_new_progress())
    delete_id=save.get_points()[-1].save_id
    check('explicit_delete_frees_capacity',save.delete_point(delete_id) and len(save.get_points())==49)
    check('new_initial_node_uses_freed_capacity',save.start_new_progress() and len(save.get_points())==50)
    check('restore_for_restart',save.load_point(completed))
    hud.toggle_inventory()
    command(world,pc,'Shot showui filename="'+(out/'restored.png').as_posix()+'"')
    yield delay(.5)
    levels.editor_request_end_play(); yield wait(lambda: not levels.is_in_play_in_editor()); yield delay(1)
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor)
    world2=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world2,0))); yield delay(1)
    pawn2=unreal.GameplayStatics.get_player_pawn(world2,0); pc2=unreal.GameplayStatics.get_player_controller(world2,0)
    command(world2,pc2,'Hearthward.Companion.CreateTest')
    save2=sub(unreal.HearthwardSaveSubsystem,world2); storage2=sub(unreal.HearthwardStorageSubsystem,world2)
    check('new_world_reload_pool',save2.enable_prototype() and len(save2.get_points())==50)
    check('disk_restore_after_pie_restart',save2.load_point(completed))
    comp2=unreal.GameplayStatics.get_all_actors_with_tag(world2,'Hearthward.Companion.PROTOTYPE_ONLY')[0]
    check('restart_restores_world_inventory_knowledge',storage2.get_item_count('wood')==7 and pawn2.get_component_by_class(unreal.HearthwardInventoryComponent).get_item_count('wood')==7 and comp2.get_delivered()==4 and comp2.source.get_item_count('wood')==12 and save2.get_knowledge()==saved_knowledge)
    yield delay(6)
    check('completed_task_not_reissued_after_restart',storage2.get_item_count('wood')==7 and comp2.get_phase()==P.COMPLETED)
    match=re.search(r'-HearthwardSaveTestPool=([0-9a-fA-F-]+)',unreal.SystemLibrary.get_command_line())
    assert match,'Test must use an isolated pool'
    pool_path=Path(unreal.Paths.project_saved_dir())/'SaveGames/HearthwardPrototype'/('test-'+uuid.UUID(match.group(1)).hex+'.hws')
    original=pool_path.read_bytes(); corrupt=bytearray(original); corrupt[-1]^=1
    previous_epoch=storage2.get_timeline_epoch()
    try:
        pool_path.write_bytes(corrupt)
        check('corrupt_load_preserves_live_world',not save2.load_point(completed) and storage2.get_item_count('wood')==7 and same(previous_epoch,storage2.get_timeline_epoch()) and save2.get_knowledge()==saved_knowledge)
        check('corrupt_pool_not_silently_reinitialized',not save2.save_point(True) and pool_path.read_bytes()==corrupt)
    finally:
        pool_path.write_bytes(original)
    check('valid_pool_load_after_repair',save2.load_point(completed))
    report['saved_campaign']=campaign.to_string(); report['completed_save']=completed.to_string()
    levels.editor_request_end_play(); yield wait(lambda: not levels.is_in_play_in_editor())

flow=run(); pending=None
def tick(dt):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline: raise TimeoutError('waiting for '+str(predicate))
                return
        pending=next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())
def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    report['passed']=not error and bool(report['checks']) and all(report['checks'].values())
    if error: report['error']=error
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
