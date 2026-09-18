"""Real UI bindings and real Qwen; fixture world setup stays outside saved maps."""
import json,time,traceback,os
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/'Saved/Task015'
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{},'model':'Qwen3.5-4B Q4_K_M / llama.cpp b10964','backend':'Vulkan 32 layers','world_source':'TASK-012 runtime fixture','ui_input':'Widget methods; physical keys separately recorded','screenshots':[]}
st={}
P=unreal.HearthwardCompanionPhase
def check(name,condition):
    report['checks'][name]=bool(condition)
    if not condition: raise AssertionError(name)
def wait(predicate,timeout=120): return predicate,time.monotonic()+timeout
def delay(seconds):
    until=time.monotonic()+seconds
    return wait(lambda: time.monotonic()>=until)
def capture(name):
    path=(out/(name+'.png')).as_posix()
    unreal.SystemLibrary.execute_console_command(st['world'],'Shot showui filename="'+path+'"',st['pc'])
    report['screenshots'].append(path)
def run():
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda: bool(unreal.GameplayStatics.get_player_pawn(world,0)))
    yield delay(1)
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    pc=unreal.GameplayStatics.get_player_controller(world,0); hud=pc.get_hud()
    ai=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==world)
    storage=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer()==world)
    st.update(world=world,pc=pc,pawn=pawn)
    check('no_autospawn_or_model',not unreal.GameplayStatics.get_all_actors_with_tag(world,'Hearthward.Companion.PROTOTYPE_ONLY') and ai.get_server_process_id()==0)
    hud.toggle_dialogue()
    check('absent_companion_cannot_open',not hud.is_dialogue_open())
    unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc)
    comp=unreal.GameplayStatics.get_all_actors_with_tag(world,'Hearthward.Companion.PROTOTYPE_ONLY')[0]
    pawn.set_actor_location(unreal.Vector(-450,-200,90),False,True)
    comp.set_actor_location(unreal.Vector(0,400,90),False,True)
    comp.camp.set_actor_location(unreal.Vector(0,400,90),False,True)
    comp.source.get_owner().set_actor_location(unreal.Vector(600,400,90),False,True)
    hud.toggle_inventory(); hud.toggle_dialogue()
    check('inventory_blocks_dialogue',hud.is_inventory_open() and not hud.is_dialogue_open())
    hud.toggle_inventory(); hud.toggle_dialogue()
    yield delay(.5)
    widget=hud.get_dialogue_widget()
    check('dialogue_opens_without_pause',hud.is_dialogue_open() and not unreal.GameplayStatics.is_game_paused(world))
    check('draft_focus_and_cursor',widget.has_draft_focus() and pc.get_editor_property('show_mouse_cursor'))
    check('movement_and_look_gated',pc.is_move_input_ignored() and pc.is_look_input_ignored())
    check('empty_send_disabled',not widget.is_send_enabled())
    widget.set_draft(' ')
    check('blank_send_disabled',not widget.is_send_enabled() and not hud.submit_dialogue(' '))
    widget.set_draft('字'*1001)
    check('oversize_disabled',not widget.is_send_enabled() and not hud.submit_dialogue('字'*1001))
    widget.set_draft('你好')
    check('valid_send_enabled',widget.is_send_enabled())
    inv=pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    inv.try_add('wood',3)
    check('weight_reads_live_inventory',format(inv.get_weight(),'.2f') in hud.get_dialogue_weight())
    inv.try_remove('wood',3)
    widget.set_draft('帮我收集两份木材，运回营地仓库。'); widget.send_draft()
    yield delay(.3)
    check('real_request_pending',ai.is_busy() and ai.get_server_process_id()>0)
    check('duplicate_send_blocked',not widget.is_send_enabled() and not hud.submit_dialogue('再收集两份木材'))
    check('thinking_visible',widget.get_displayed_status()==ai.get_status() and ('加载' in ai.get_status() or '思考' in ai.get_status()))
    capture('thinking')
    yield wait(lambda: not ai.is_busy())
    check('real_model_collect',bool(ai.get_last_structured_result()) and json.loads(ai.get_last_structured_result())['intent']=='collect')
    yield delay(.3)
    check('reply_binding',widget.get_displayed_reply()==ai.get_npc_line() and bool(widget.get_displayed_reply()))
    yield wait(lambda: comp.get_phase()==P.COMPLETED,60)
    yield delay(.3)
    check('completion_is_real',comp.get_delivered()==2 and storage.get_item_count('wood')==2)
    check('completed_feedback','目标已交付' in widget.get_displayed_progress() and '2 / 2' in widget.get_displayed_progress())
    report['real_reply']=ai.get_npc_line(); capture('completed')
    yield delay(.5)
    comp.source.try_remove('wood',comp.source.get_item_count('wood'))
    ticket=comp.request(pawn,'fixture缺料状态覆盖')
    comp.submit(pawn,ticket,'wood',2,['collect','return','deposit'])
    yield wait(lambda: comp.get_phase()==P.WAITING_AT_CAMP,30)
    yield delay(.3)
    check('blocked_feedback',bool(comp.get_editor_property('block_reason')) and comp.get_editor_property('block_reason') in widget.get_displayed_progress())
    capture('blocked'); yield delay(.5)
    hud.cancel_dialogue_task(); yield delay(.3)
    check('cancelled_feedback',comp.get_phase()==P.CANCELLED and '已取消' in widget.get_displayed_progress())
    widget.set_draft('你好。'); widget.send_draft(); hud.cancel_dialogue_reply()
    yield delay(.5)
    check('reply_cancel_feedback',not ai.is_busy() and '取消' in widget.get_displayed_status() and not widget.get_displayed_reply())
    widget.set_draft('你好。'); widget.send_draft(); storage.advance_timeline()
    yield delay(.5)
    check('stale_feedback',not ai.is_busy() and '变化' in widget.get_displayed_status() and not widget.get_displayed_reply() and comp.get_phase()==P.CANCELLED)
    capture('stale'); yield delay(.5)
    old=pawn.get_actor_location()
    pawn.set_actor_location(comp.get_actor_location()+unreal.Vector(3500,0,0),False,True)
    yield delay(.3)
    check('range_invalidates_ui',not widget.is_send_enabled() and '30米' in widget.get_displayed_status() and not widget.get_displayed_reply())
    hud.close_dialogue(); hud.toggle_dialogue()
    check('out_of_range_cannot_reopen',not hud.is_dialogue_open())
    pawn.set_actor_location(old,False,True); hud.toggle_dialogue(); yield delay(.3)
    widget=hud.get_dialogue_widget(); widget.set_draft('你好。'); widget.send_draft(); hud.close_dialogue()
    check('close_preserves_existing_pending_policy',not hud.is_dialogue_open() and ai.is_busy())
    check('close_restores_controls',not pc.is_move_input_ignored() and not pc.is_look_input_ignored() and not pc.get_editor_property('show_mouse_cursor'))
    yield wait(lambda: not ai.is_busy())
    hud.toggle_dialogue(); yield delay(.3)
    check('reopen_reads_current_reply',hud.get_dialogue_widget().get_displayed_reply()==ai.get_npc_line())
    hud.toggle_inventory()
    check('inventory_excludes_dialogue',hud.is_inventory_open() and not hud.is_dialogue_open() and unreal.GameplayStatics.is_game_paused(world))
    capture('inventory'); yield delay(.5)
    hud.toggle_inventory()
    check('inventory_resumes_world',not unreal.GameplayStatics.is_game_paused(world))
    check('ui_did_not_duplicate_stock',storage.get_item_count('wood')==2 and inv.get_item_count('wood')==0)
    hud.toggle_dialogue()
    levels.editor_request_end_play(); yield wait(lambda:not levels.is_in_play_in_editor())
    yield delay(1)
    levels.editor_request_begin_play(); yield wait(levels.is_in_play_in_editor)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    yield wait(lambda:bool(unreal.GameplayStatics.get_player_pawn(world,0)))
    pc=unreal.GameplayStatics.get_player_controller(world,0)
    check('new_pie_clears_ui_and_input',not pc.get_hud().is_dialogue_open() and not pc.is_move_input_ignored() and not pc.get_editor_property('show_mouse_cursor'))
    if os.environ.get('TASK015_INTERACTIVE')=='1':
        unreal.SystemLibrary.execute_console_command(world,'Hearthward.Companion.CreateTest',pc)
        st.update(world=world,pc=pc,pawn=unreal.GameplayStatics.get_player_pawn(world,0))

def finish(error=None):
    if error: report['error']=error
    report['ok']=not error and bool(report['checks']) and all(report['checks'].values())
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if report['ok'] and os.environ.get('TASK015_INTERACTIVE')=='1':
        st['watch']=unreal.register_slate_post_tick_callback(observe_interactive)
    elif levels.is_in_play_in_editor(): levels.editor_request_end_play()
def observe_interactive(dt):
    if not levels.is_in_play_in_editor(): return
    pc=st['pc']; hud=pc.get_hud(); pawn=st['pawn']; widget=hud.get_dialogue_widget()
    ai=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==st['world'])
    pos=pawn.get_actor_location()
    data={'dialogue_open':hud.is_dialogue_open(),'inventory_open':hud.is_inventory_open(),'move_ignored':pc.is_move_input_ignored(),'focus':widget.has_draft_focus() if widget else False,'status':widget.get_displayed_status() if widget else '', 'reply':widget.get_displayed_reply() if widget else '', 'progress':widget.get_displayed_progress() if widget else '', 'server_pid':ai.get_server_process_id(),'position':[pos.x,pos.y,pos.z]}
    if time.monotonic()-st.get('last_snapshot',0)>1:
        (out/'interactive-state.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
        st['last_snapshot']=time.monotonic()
    key=(data['dialogue_open'],data['inventory_open'],data['status'],data['reply'])
    if key!=st.get('last_visual'):
        st['last_visual']=key
        st['visual_index']=st.get('visual_index',0)+1
        capture('interactive-'+str(st['visual_index']))
flow=run(); pending=None; started=time.monotonic()
def tick(dt):
    global pending
    try:
        if time.monotonic()-started>300: raise TimeoutError('UI test deadline')
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError('UI state wait')
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
