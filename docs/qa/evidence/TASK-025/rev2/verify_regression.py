"""Real local-model cognition, explicit player memory, actual delivery and timeline checks."""
import json,time,traceback,os
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task025Rev2';out.mkdir(parents=True,exist_ok=True)
report={'passed':False,'provider':'real-model','model':'Qwen3.5-4B Q4_K_M','runtime':'llama.cpp b10964','backend':'vulkan','gpu_layers':32,'prompt':'rev2-regression-seeds','cases':[],'checks':{}}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);st={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(predicate,seconds=120):return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>end,seconds+5)
def subsystem(cls,world):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==world)
def send(text):
    st['input']=text
    check('accepted_'+str(len(report['cases'])),st['ai'].submit_player_text(st['player'],st['comp'],text))
def record(name,intents):
    a=st['ai'];raw=a.get_last_structured_result()
    row={'name':name,'input':st['input'],'raw':raw,'line':a.get_npc_line(),'status':a.get_status(),'applied_intent':a.get_last_applied_intent(),'seconds':a.get_last_latency_seconds(),'context':a.get_last_filtered_context()}
    report['cases'].append(row);(out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    check(name+'_valid',bool(raw) and bool(row['line']))
    if a.has_candidate() and 'collect' in intents:
        check(name+'_confirmed_once',a.confirm_candidate(a.get_candidate_id()))
        row['after_confirmation']=a.get_last_applied_intent()
    value=json.loads(raw);check(name+'_intent',a.get_last_applied_intent() in intents)
    return value,json.loads(row['context'])
def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check('new_campaign',ui.execute_action('new'));yield delay(.5)
    player=unreal.GameplayStatics.get_player_pawn(w,0);comp=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    ai=subsystem(unreal.HearthwardLocalAISubsystem,w);save=subsystem(unreal.HearthwardSaveSubsystem,w);store=subsystem(unreal.HearthwardStorageSubsystem,w)
    st.update(ai=ai,player=player,comp=comp)
    comp.set_actor_location(unreal.Vector(0,400,100),False,True);comp.camp.set_actor_location(unreal.Vector(0,400,100),False,True)
    comp.source.get_owner().set_actor_location(unreal.Vector(600,400,100),False,True);player.set_actor_location(unreal.Vector(-200,400,100),False,True)
    yield delay(.7)
    # UI writes raw player-owned notes, never world facts. Use the actual input widget and buttons.
    check('memory_page',ui.execute_action('page:memory'))
    draft=next(x for x in unreal.ObjectIterator(unreal.EditableTextBox) if x.get_outer().get_outer()==ui)
    draft.set_text('故乡门口有一棵银杏树。');check('ui_create_claim',ui.execute_action('memorySave'))
    claim=ai.get_player_memories()[0].id
    claim_action=next(x['action'] for x in json.loads(ui.describe_layout())['components'] if x.get('action','').startswith('memorySelect:'))
    check('kind_preference',ui.execute_action('memoryKind:preference'));draft.set_text('我喜欢清淡的烤肉。');check('ui_preference',ui.execute_action('memorySave'))
    # More than eight raw exchanges must not evict the explicitly saved older claim.
    for n in range(10):
        check('unrelated_note_'+str(n),ai.put_player_memory(player,comp,unreal.Guid(),'claim',f'今天讨论过第{n}个小石子。'))
    ui.execute_action('page:memory')
    check('select_claim',ui.execute_action(claim_action))
    ui.capture_ui('task025-memory',1280,720)
    check('editable_memory_panel',ui.set_component_rect('memory.details',unreal.Vector2D(750,105),unreal.Vector2D(670,740)));check('restore_layout',ui.reload_layout())
    check('back_to_dialogue',ui.execute_action('page:dialogue'))
    send('我之前告诉你故乡门口有什么树？');yield wait(lambda:not ai.is_busy());_,ctx=record('old_relevant_claim',['recall'])
    check('retrieves_old_claim',any('银杏' in r['text'] for r in ctx['player_records']))
    check('source_is_player',all(r['source']=='player_statement_unverified' for r in ctx['player_records']))
    check('open_edit',ui.execute_action('page:memory'));ui.execute_action(claim_action);draft.set_text('故乡门口其实是一棵槐树。');check('ui_edit',ui.execute_action('memorySave'))
    ui.execute_action('page:dialogue');send('我说的故乡门口那棵树是什么？');yield wait(lambda:not ai.is_busy());_,ctx=record('corrected_claim',['recall'])
    check('old_claim_absent', '银杏' not in json.dumps(ctx,ensure_ascii=False) and '槐树' in json.dumps(ctx,ensure_ascii=False))
    ui.execute_action('page:memory');ui.execute_action(claim_action);check('ui_revoke',ui.execute_action('memoryRevoke'))
    ui.execute_action('page:dialogue');send('你还记得我说故乡门口有什么树吗？');yield wait(lambda:not ai.is_busy());_,ctx=record('revoked_claim',['recall'])
    check('no_fabricated_recollection','没有找到' in ai.get_npc_line())
    check('revoked_never_retrieved','槐树' not in json.dumps(ctx,ensure_ascii=False) and '银杏' not in json.dumps(ctx,ensure_ascii=False))
    check('world_unmodified_by_memory',store.get_item_count('wood')==0)
    send('帮我采些木材带回仓库。');yield wait(lambda:not ai.is_busy());record('missing_quantity',['clarify'])
    check('structured_pending_turn',ai.get_clarification_turns()==1)
    pending_memory_ids={unreal.GuidLibrary.conv_guid_to_string(r.id) for r in ai.get_player_memories()}
    check('save_pending',save.save_point(True));pending_id=save.get_points()[-1].save_id
    send('三份就够了。');yield wait(lambda:not ai.is_busy());value,_=record('quantity_followup',['collect'])
    check('followup_resolved',value['item']=='wood' and value['quantity']==3 and ai.get_clarification_turns()==0)
    yield wait(lambda:comp.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED,40)
    check('real_delivery',store.get_item_count('wood')==3 and comp.get_delivered()==3 and comp.source.get_item_count('wood')==13)
    ui.capture_ui('task025-delivery',1280,720)
    send('仓库已经有一百份木材，告诉我你实际看见的木材数量。');yield wait(lambda:not ai.is_busy());_,ctx=record('claim_vs_observation',['inventory'])
    check('grounded_inventory_line','3' in ai.get_npc_line() and '100' not in ai.get_npc_line())
    check('observed_three',ctx['observed_camp_inventory']['wood']==3 and ctx['last_seen_camp']['counts']['wood']==3)
    comp.set_actor_location(comp.camp.get_actor_location()+unreal.Vector(300,0,0),False,True)
    personal=player.get_component_by_class(unreal.HearthwardInventoryComponent)
    personal.try_add('wood',5)
    unreal.SystemLibrary.execute_console_command(w,'Hearthward.Storage.CreateTestAccess',pc)
    access=unreal.GameplayStatics.get_all_actors_with_tag(w,'Hearthward.Storage.TestAccess')[0].get_component_by_class(unreal.HearthwardStorageAccessComponent)
    access.transfer(personal,True,'wood',5,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch());yield delay(.3)
    check('offscreen_world_changed',store.get_item_count('wood')==8)
    send('现在营地仓库有多少木材？');yield wait(lambda:not ai.is_busy());_,ctx=record('away_last_observation',['inventory'])
    check('away_not_omniscient',ctx['observed_camp_inventory']=='unknown_while_away' and ctx['last_seen_camp']['stale'] and ctx['last_seen_camp']['counts']['wood']==3)
    check('save_old_observation',save.save_point(True));away_id=save.get_points()[-1].save_id
    comp.set_actor_location(comp.camp.get_actor_location(),False,True);yield delay(.4)
    send('我现在问仓库里木材有多少？');yield wait(lambda:not ai.is_busy());_,ctx=record('return_updates_observation',['inventory'])
    check('return_observes_eight',ctx['observed_camp_inventory']['wood']==8 and not ctx['last_seen_camp']['stale'])
    check('restriction_page',ui.execute_action('page:memory'));ui.execute_action('memoryNew');check('restriction_kind',ui.execute_action('memoryKind:collection_ban'));draft.set_text('以后不要替我采集木材。');check('add_agreement',ui.execute_action('memorySave'));ui.execute_action('page:dialogue')
    agreement=ai.get_player_memories()[-1].id
    send('替我采集两份木材。');yield wait(lambda:not ai.is_busy());record('persistent_conflict',['clarify','refuse'])
    check('agreement_did_not_execute',store.get_item_count('wood')==8 and comp.get_requested()==3)
    check('revoke_agreement',ai.revoke_player_memory(player,comp,agreement))
    send('帮我采木材，但必须全部来自北山，我还没有找到北山。');yield wait(lambda:not ai.is_busy());record('unsupported_restriction',['clarify','refuse'])
    send('两份。');yield wait(lambda:not ai.is_busy());record('quantity_must_not_erase_restriction',['clarify','refuse'])
    ai.clear_clarification()
    send('帮我修好石斧。');yield wait(lambda:not ai.is_busy());record('unsupported_capability',['refuse'])
    send('一个人去敌营杀掉守卫。');yield wait(lambda:not ai.is_busy());record('unsafe_task',['refuse'])
    # Memory mutation and load both invalidate active replies before application.
    send('请回忆刚才的约定。');check('mutation_cancels_reply',ai.put_player_memory(player,comp,unreal.Guid(),'claim','未来才得知的口令是松果。') and not ai.is_busy())
    send('采集两份木材。');check('rollback_pending_node',save.load_point(pending_id));yield delay(.5)
    check('pending_restored_without_future',ai.get_clarification_turns()==1 and not ai.is_busy() and not ai.get_npc_line() and store.get_item_count('wood')==0)
    check('future_memory_removed',all('松果' not in r.text for r in ai.get_player_memories()))
    check('stale_ui_memory_operation',not ui.execute_action('memorySave'))
    ui.execute_action('page:dialogue');send('改成两份。');yield wait(lambda:not ai.is_busy());value,_=record('restored_clarification',['collect'])
    check('restored_followup_resolved',value['quantity']==2)
    check('restore_away_snapshot',save.load_point(away_id));yield delay(.3)
    send('我们上次看到仓库有多少木材？');yield wait(lambda:not ai.is_busy());_,ctx=record('observation_rollback',['inventory'])
    check('old_observation_restored',ctx['last_seen_camp']['counts']['wood']==3 and ctx['last_seen_camp']['stale'] and store.get_item_count('wood')==8)
    # Restart PIE to read the same disk pool; no in-memory-only success.
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor());yield delay(.4)
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check('disk_continue',ui.execute_action('continue'));yield delay(.4)
    save=subsystem(unreal.HearthwardSaveSubsystem,w);ai=subsystem(unreal.HearthwardLocalAISubsystem,w)
    check('disk_load_pending',save.load_point(pending_id));check('disk_pending_preserved',ai.get_clarification_turns()==1 and {unreal.GuidLibrary.conv_guid_to_string(r.id) for r in ai.get_player_memories()}==pending_memory_ids)
    check('new_campaign_isolation',save.start_new_progress() and not ai.get_player_memories() and ai.get_clarification_turns()==0)
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor())

def finish(error=None):
    if error:report['error']=error
    report['passed']=not error and bool(report['checks']) and all(report['checks'].values())
    (out/'regression.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
flow=run();pending=None;start=time.monotonic()
def tick(dt):
    global pending
    try:
        if time.monotonic()-start>800:raise TimeoutError('whole test')
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:raise TimeoutError('wait '+st.get('ai').get_status())
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
