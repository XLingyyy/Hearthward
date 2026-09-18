"""Real interaction timer/containers/disk; two PIE worlds, isolated save pool."""
import json, os, re, time, traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task018'; out.mkdir(parents=True,exist_ok=True)
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
    components(); s=state; c=s['companion']; interaction=s['interaction']; action=s['action']; save=s['save']; bag=s['bag']; source=s['source']; w=s['world']; hud=s['pc'].get_hud()
    check('finite_existing_fixture',counts()==(16,0,0,0))
    check('explicit_enable',save.enable_prototype() and save.start_new_progress())
    initial=save.get_points()[0].save_id
    approach(source.get_owner()); yield delay(.5)
    target=interaction.get_nearest_target()
    check('source_selected_in_range',target and target.get_owner()==source.get_owner())
    capture('gather-prompt'); yield delay(.3)
    check('start_via_nearest',interaction.interact_nearest())
    check('repeat_key_does_not_restart',not interaction.interact_nearest())
    yield delay(1)
    check('no_early_reward',counts()==(16,0,0,0))
    check('save_during_interaction_rejected',not save.save_point(True) and '交互' in save.get_status())
    hud.toggle_inventory(); elapsed=action.get_elapsed_seconds(); yield delay(1)
    check('pause_freezes_timer',abs(action.get_elapsed_seconds()-elapsed)<.01 and counts()==(16,0,0,0))
    hud.toggle_inventory(); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('gather_one_real_wood',counts()==(15,1,0,0) and '已放入背包' in interaction.get_completion_feedback())
    capture('gathered'); yield delay(.5)
    yield delay(1); check('completion_not_repeated',counts()==(15,1,0,0))
    check('start_damage_case',interaction.interact_nearest()); yield delay(.3)
    unreal.GameplayStatics.apply_damage(s['pawn'],1,s['pc'],c,unreal.DamageType)
    yield delay(.3)
    check('damage_cancels_without_consumption',action.get_status()==unreal.HearthwardTimedActionStatus.INTERRUPTED and counts()==(15,1,0,0))
    check('start_movement_case',interaction.interact_nearest()); yield delay(.3)
    s['pawn'].add_movement_input(unreal.Vector(1,0,0),1,False); yield delay(.5)
    check('movement_cancels_without_consumption',action.get_status()==unreal.HearthwardTimedActionStatus.INTERRUPTED and counts()==(15,1,0,0))
    approach(source.get_owner()); yield delay(.5)
    bag.try_add('stone',99)
    check('start_full_case',interaction.interact_nearest()); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('full_capacity_atomic',counts()==(15,1,0,0) and '容量不足' in interaction.get_completion_feedback())
    capture('full'); yield delay(.3); bag.try_remove('stone',99)
    approach(c.camp); yield delay(.5)
    check('camp_selected',interaction.get_nearest_target().get_owner()==c.camp)
    check('start_deposit',interaction.interact_nearest()); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('deposit_real_transfer',counts()==(15,0,1,0) and '已入库' in interaction.get_completion_feedback())
    capture('deposited'); yield delay(.3)
    check('save_after_loop',save.save_point(True)); saved=save.get_points()[-1].save_id
    check('start_empty_deposit',interaction.interact_nearest()); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('empty_deposit_no_duplication',counts()==(15,0,1,0) and '没有' in interaction.get_completion_feedback())
    approach(source.get_owner()); yield delay(.5); check('start_before_load',interaction.interact_nearest()); yield delay(.5)
    check('load_cancels_pending',save.load_point(saved)); yield delay(5.5)
    check('old_timer_cannot_reward_after_load',counts()==(15,0,1,0) and interaction.get_status()==unreal.HearthwardInteractionStatus.IDLE and not interaction.get_completion_feedback())
    approach(source.get_owner()); yield delay(.5)
    check('gather_after_restore',interaction.interact_nearest()); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('restored_component_still_usable',counts()==(14,1,1,0))
    check('load_restores_all_containers',save.load_point(saved) and counts()==(15,0,1,0))
    # Actual companion takes the remaining single resource while the player timer is pending.
    source.try_remove('wood',14); approach(source.get_owner()); yield delay(.5)
    c.set_actor_location(source.get_owner().get_actor_location(),False,True)
    ticket=c.request(s['pawn'],'PROTOTYPE_ONLY shared resource contention')
    c.submit(s['pawn'],ticket,'wood',1,['collect','return','deposit'])
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.GATHERING); yield delay(1)
    check('player_starts_competing_action',interaction.interact_nearest())
    yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('companion_competition_no_duplicate',source.get_item_count('wood')==0 and bag.get_item_count('wood')==0 and '耗尽' in interaction.get_completion_feedback() and s['storage'].get_item_count('wood')+c.bag.get_item_count('wood')==2)
    capture('exhausted'); yield delay(.3)
    check('restore_after_competition',save.load_point(saved) and counts()==(15,0,1,0))
    approach(source.get_owner()); yield delay(.5); check('start_destroy_case',interaction.interact_nearest())
    source.get_owner().destroy_actor(); yield delay(.5)
    check('destroy_target_cancels',interaction.get_status()==unreal.HearthwardInteractionStatus.INVALID_TARGET and bag.get_item_count('wood')==0)
    levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor()); yield delay(1)
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor); yield delay(2)
    components(); s=state; interaction=s['interaction']; action=s['action']; save=s['save']
    check('new_pie_clean_fixture',counts()==(16,0,0,0))
    check('disk_restore_second_pie',save.enable_prototype() and save.load_point(saved) and counts()==(15,0,1,0))
    approach(s['source'].get_owner()); yield delay(.5)
    check('second_pie_e_path',interaction.interact_nearest()); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('second_pie_gather_works',counts()==(14,1,1,0))
    approach(s['camp']); yield delay(.5); interaction.interact_nearest(); yield wait(lambda:action.get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('second_pie_deposit_works',counts()==(14,0,2,0))
    check('second_pie_save',save.save_point(True))
    capture('restored-loop'); yield delay(.5)
    if os.environ.get('TASK018_INTERACTIVE')=='1':
        approach(s['source'].get_owner()); yield delay(.5)
    else:
        levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor())
def run_camera():
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor); yield delay(2)
    components(); s=state
    approach(s['source'].get_owner()); yield delay(.5)
    check('final_build_gather',s['interaction'].interact_nearest())
    yield wait(lambda:s['action'].get_status()==unreal.HearthwardTimedActionStatus.COMPLETED)
    check('final_build_quantity',counts()==(15,1,0,0))
    camp=s['camp'].get_actor_location()
    s['pawn'].set_actor_location(unreal.Vector(camp.x+85,camp.y-21,camp.z),False,True)
    s['pc'].set_control_rotation(unreal.Rotator(0,0,0)); yield delay(1)
    camera=s['pc'].player_camera_manager.get_camera_location()
    distance=(camera-s['pawn'].get_actor_location()).length()
    report['camera_distance_cm']=distance
    check('camera_not_pushed_into_player',distance>350)
    check('camp_target_after_camera_fix',s['interaction'].get_nearest_target().get_owner()==s['camp'])
    capture('camera-fixed'); yield delay(.5)
def observe(dt):
    if not levels.is_in_play_in_editor() or time.monotonic()-state.get('last',0)<.35:return
    state['last']=time.monotonic(); pos=state['pawn'].get_actor_location()
    data={'counts':counts(),'position':[pos.x,pos.y,pos.z],'interaction':str(state['interaction'].get_status()),'feedback':state['interaction'].get_completion_feedback(),'paused':unreal.GameplayStatics.is_game_paused(state['world'])}
    (out/'interactive-state.json').write_text(json.dumps(data,ensure_ascii=False),encoding='utf-8')
    if (Path(unreal.Paths.project_dir())/'.agent-local/task018-record').exists():
        frames=out/'physical-frames'; frames.mkdir(exist_ok=True)
        i=state.get('frame',0);state['frame']=i+1
        command('Shot showui filename="'+(frames/('frame-%05d.png'%i)).as_posix()+'"')
        with (out/'physical-trace.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(dict(time=time.monotonic(),frame=i,**data),ensure_ascii=False)+'\n')
def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    report['passed']=not error and bool(report['checks']) and all(report['checks'].values())
    if error:report['error']=error
    (out/('camera-verification.json' if os.environ.get('TASK018_CAMERA_ONLY')=='1' else 'verification.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    if os.environ.get('TASK018_INTERACTIVE')=='1' and report['passed']:state['watch']=unreal.register_slate_post_tick_callback(observe)
    elif levels.is_in_play_in_editor():levels.editor_request_end_play()
flow=run_camera() if os.environ.get('TASK018_CAMERA_ONLY')=='1' else run();pending=None
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
