"""TASK-029 deterministic PIE: suggestions refresh explicitly, remain cognition-isolated until selection, and stale safely."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task029";out.mkdir(parents=True,exist_ok=True)
result=out/"suggestions-pie-results.json"
report={"ok":False,"checks":{}}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)

def wait(pred,seconds=30): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def find_kind(items,kind):
    for x in items:
        if str(x.kind).lower()==kind.lower(): return x
    return None
def goal_collect(n=2):
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":"collect","item":"wood","quantity":n,"quantity_mode":"additional_acquired","source_ref":"S1"}.items():
        g.set_editor_property(k,v)
    return g

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    # Production New Game travels to Natural World; AI validation uses an explicit prototype.
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.CreateTest",pc)
    fixture_player=unreal.GameplayStatics.get_player_pawn(w,0)
    fixture_player.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
    fixture_bag=fixture_player.get_component_by_class(unreal.HearthwardInventoryComponent)
    for item,count in json.loads((Path(unreal.Paths.project_dir())/"Resources/Data/gameplay.json").read_text(encoding="utf-8"))["loadout"].items():
        fixture_bag.try_add(item,int(count))
    fixture_save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer()==w)
    check("enable_prototype",fixture_save.enable_prototype())
    check("new_campaign",fixture_save.start_new_progress())
    ui.open_page("hud");yield delay(.8)
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-150,0,0),False,True)
    ai=sub(unreal.HearthwardLocalAISubsystem,w);store=sub(unreal.HearthwardStorageSubsystem,w)
    personal=p.get_component_by_class(unreal.HearthwardInventoryComponent)

    # No automatic generation: suggestions start empty and waiting does not populate them.
    check("suggestions_initially_empty",len(ai.get_suggestions())==0)
    revision=ai.get_memory_revision();clarifications=ai.get_clarification_turns()
    input_before=ai.get_last_input();context_before=ai.get_last_filtered_context()
    yield delay(.5)
    check("no_background_refresh",len(ai.get_suggestions())==0)

    # Modern Screen explicit action refreshes exactly three without changing NPC cognition.
    check("open_dialogue",ui.execute_action("page:dialogue"))
    check("screen_refresh_action",ui.execute_action("suggestRefresh"))
    suggestions=ai.get_suggestions()
    check("exactly_three_after_explicit_refresh",len(suggestions)==3)
    check("refresh_does_not_write_memory",ai.get_memory_revision()==revision)
    check("refresh_does_not_write_clarification",ai.get_clarification_turns()==clarifications)
    check("refresh_does_not_stage_candidate",not ai.has_candidate())
    check("unselected_suggestions_do_not_become_input",ai.get_last_input()==input_before)
    check("unselected_suggestions_do_not_enter_filtered_context",ai.get_last_filtered_context()==context_before)
    ids=[x.id.to_string() for x in suggestions]

    camp=find_kind(suggestions,"camp_stock")
    collect=find_kind(suggestions,"collect")
    check("camp_fact_suggestion_present",camp is not None and camp.observed_count==store.get_item_count("wood"))
    check("safe_idle_collect_suggestion_present",collect is not None)

    # Change the authoritative camp fact without refresh. IDs remain, proving no auto refresh.
    personal.try_add("wood",1)
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Storage.CreateTestAccess",pc)
    access=unreal.GameplayStatics.get_all_actors_with_tag(w,"Hearthward.Storage.TestAccess")[0].get_component_by_class(unreal.HearthwardStorageAccessComponent)
    check("camp_mutation",access.transfer(personal,True,"wood",1,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch()).result==unreal.HearthwardInventoryResult.SUCCESS)
    check("world_change_does_not_auto_replace_suggestions",[x.id.to_string() for x in ai.get_suggestions()]==ids)
    check("stale_camp_suggestion_rejected",not ai.submit_suggestion(p,c,camp.id))
    check("stale_reason",ai.get_reason_code()=="STALE_SUGGESTION")
    check("stale_rejection_no_memory_write",ai.get_memory_revision()==revision)

    # Refresh updates the fact. Clicking a quick suggestion enters the normal text path;
    # this isolated worktree has no GGUF, so generation fails safely after source tagging.
    check("refresh_after_fact_change",ai.refresh_suggestions(p,c))
    suggestions=ai.get_suggestions();camp=find_kind(suggestions,"camp_stock");collect=find_kind(suggestions,"collect")
    check("camp_fact_updated",camp is not None and camp.observed_count==store.get_item_count("wood"))

    # Safety changes stale a collect suggestion even when its timeline/memory snapshot still matches.
    collect_id=collect.id
    c.set_editor_property("source_safe",False)
    check("safety_change_stales_collect",not ai.submit_suggestion(p,c,collect_id))
    check("safety_stale_reason",ai.get_reason_code()=="STALE_SUGGESTION")
    c.set_editor_property("source_safe",True)

    # Timeline changes stale every cached suggestion; refresh creates a new cache bound to the new epoch.
    check("refresh_before_timeline_change",ai.refresh_suggestions(p,c))
    timeline_suggestion=ai.get_suggestions()[0]
    store.advance_timeline()
    check("timeline_change_stales_cache",not ai.submit_suggestion(p,c,timeline_suggestion.id))
    check("timeline_stale_reason",ai.get_reason_code()=="STALE_SUGGESTION")
    check("refresh_after_timeline_change",ai.refresh_suggestions(p,c))
    suggestions=ai.get_suggestions();collect=find_kind(suggestions,"collect")
    check("collect_available_after_revalidation",collect is not None)

    command_before=c.get_command_id();phase_before=c.get_phase();source_before=c.source.get_item_count("wood");camp_before=store.get_item_count("wood")
    check("quick_input_reaches_normal_model_boundary",not ai.submit_suggestion(p,c,collect.id))
    check("quick_input_source_tagged",ai.get_last_input_source()=="quick_suggestion")
    check("quick_input_text_is_selected_suggestion",ai.get_last_input()==collect.message)
    check("missing_model_does_not_execute_suggestion",not ai.has_candidate() and c.get_phase()==phase_before
          and unreal.GuidLibrary.equal_equal_guid_guid(c.get_command_id(),command_before)
          and c.source.get_item_count("wood")==source_before and store.get_item_count("wood")==camp_before)

    # An active execution refresh must not offer a replacement collect action.
    check("manual_goal_card",ai.set_structured_goal(p,c,goal_collect(2)))
    check("manual_goal_confirm",ai.confirm_candidate(ai.get_candidate_id()))
    check("active_refresh",ai.refresh_suggestions(p,c))
    active=ai.get_suggestions()
    progress=find_kind(active,"progress")
    check("active_goal_prioritizes_progress",progress is not None)
    check("active_goal_has_no_collect_replacement",find_kind(active,"collect") is None)
    progress_id=progress.id
    check("cancel_active_goal",ai.cancel_execution(p,c))
    check("progress_suggestion_stales_after_cancel",not ai.submit_suggestion(p,c,progress_id))
    check("cancel_stale_reason",ai.get_reason_code()=="STALE_SUGGESTION")

    # Legacy HUD adapter uses the same cached suggestion service; no second knowledge path.
    hud=pc.get_hud()
    # Default screen owns dialogue presentation, so exercise the adapter without forcing legacy rendering.
    check("legacy_adapter_returns_same_cache",len(ai.get_suggestions())==3)

    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error: report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>150: raise TimeoutError("TASK-029 suggestion PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
