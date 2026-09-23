"""TASK-030 PIE: high-level companion directives stay confirmation-gated while UE owns tactics."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task030";out.mkdir(parents=True,exist_ok=True)
result=out/"combat-policy-pie-results.json"
report={"ok":False,"checks":{}}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)

def wait(pred,seconds=30): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)

def goal(intent,item,quantity,mode,source):
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":intent,"item":item,"quantity":quantity,"quantity_mode":mode,"source_ref":source}.items():
        g.set_editor_property(k,v)
    return g

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check("new_campaign",ui.execute_action("new"));yield delay(.5)

    p=unreal.GameplayStatics.get_player_pawn(w,0)
    companions=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)
    check("companion_fixture_present",len(companions)==1)
    c=companions[0]
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    gameplay=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    ai=sub(unreal.HearthwardLocalAISubsystem,w)
    check("adventure_enabled",gameplay.enabled)

    # Legacy direct controls are preserved and now flow through the same directive boundary.
    check("legacy_follow",gameplay.order_companion("follow"))
    yield delay(.2)
    check("legacy_follow_order",str(gameplay.companion_order).lower()=="follow")
    check("legacy_follow_tactic",str(gameplay.get_companion_tactical_intent()).lower()=="follow")

    check("legacy_wait",gameplay.order_companion("wait"))
    yield delay(.1)
    check("legacy_wait_order",str(gameplay.companion_order).lower()=="wait")
    check("legacy_wait_tactic",str(gameplay.get_companion_tactical_intent()).lower()=="hold")

    # Natural-language equivalent stays candidate-gated: staging assist does not alter order.
    assist=goal("companion_order","assist",1,"directive","player")
    before=str(gameplay.companion_order).lower()
    check("assist_candidate_staged",ai.set_structured_goal(p,c,assist))
    check("assist_has_candidate",ai.has_candidate())
    check("assist_no_effect_before_confirm",str(gameplay.companion_order).lower()==before)
    assist_id=ai.get_candidate_id()
    check("assist_confirm",ai.confirm_candidate(assist_id))
    check("assist_maps_to_legacy_attack",str(gameplay.companion_order).lower()=="attack")
    yield delay(.6)

    # Encounter data places guard_1 nearest to camp/player origin; UE policy chooses it, not the model.
    tactic=str(gameplay.get_companion_tactical_intent()).lower()
    target=str(gameplay.get_companion_combat_target()).lower()
    report["assist_tactic"]=tactic;report["assist_target"]=target;report["assist_reason"]=gameplay.get_companion_combat_reason()
    check("ue_owns_tactical_assist",tactic=="assist")
    check("ue_selects_nearest_legal_threat",target=="guard_1")
    check("tactical_reason_present",len(gameplay.get_companion_combat_reason())>0)

    # Candidate becomes stale when the player leaves communication range before confirmation.
    follow=goal("companion_order","follow",1,"directive","player")
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True);yield delay(.1)
    check("follow_candidate_staged",ai.set_structured_goal(p,c,follow))
    stale_id=ai.get_candidate_id()
    order_before_stale=str(gameplay.companion_order).lower()
    p.set_actor_location(c.get_actor_location()+unreal.Vector(4000,0,0),False,True);yield delay(.1)
    check("stale_follow_rejected",not ai.confirm_candidate(stale_id))
    check("stale_follow_does_not_change_order",str(gameplay.companion_order).lower()==order_before_stale)
    check("stale_confirmation_reason",ai.get_reason_code()=="STALE_CONFIRMATION")

    # An explicitly confirmed directive may replace an active task through existing cancellation semantics.
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True);yield delay(.1)
    collect=goal("collect","wood",2,"additional_acquired","S1")
    check("collect_candidate",ai.set_structured_goal(p,c,collect))
    check("collect_confirm",ai.confirm_candidate(ai.get_candidate_id()))
    check("collect_is_active",c.get_requested()==2)
    hold=goal("companion_order","hold",1,"directive","player")
    check("hold_candidate_over_active_task",ai.set_structured_goal(p,c,hold))
    check("active_task_unchanged_before_hold_confirm",c.get_requested()==2)
    check("hold_confirm_replaces_task",ai.confirm_candidate(ai.get_candidate_id()))
    check("hold_sets_wait_order",str(gameplay.companion_order).lower()=="wait")
    check("old_task_cancelled_after_explicit_confirm",c.get_phase()==unreal.HearthwardCompanionPhase.CANCELLED)

    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error: report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()
    try: unreal.SystemLibrary.quit_editor()
    except Exception: pass

flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>120: raise TimeoutError("TASK-030 combat policy PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
