"""TASK-037 PIE: confirmed coordination behavior derives a persistent, non-authoritative suggestion prior."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task037";out.mkdir(parents=True,exist_ok=True)
result_path=out/"coordination-prior-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=30): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def profile(ai,label):
    p=ai.get_coordination_profile()
    row={"label":label,"samples":int(p.samples),"hold":int(p.hold_count),"follow":int(p.follow_count),
         "assist":int(p.assist_count),"preferred":str(p.preferred_directive),"confidence":float(p.confidence),"stable":bool(p.stable)}
    report["samples"].append(row);return p
def directive_goal(item):
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":"companion_order","item":item,"quantity":1,"quantity_mode":"directive","source_ref":"player"}.items():
        g.set_editor_property(k,v)
    return g
def save_point(save):
    before={x.save_id.to_string() for x in save.get_points()}
    check("save_profile",save.save_point(True))
    created=[x for x in save.get_points() if x.save_id.to_string() not in before]
    check("save_profile_new_id",len(created)==1)
    return created[0].save_id

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check("new_campaign",ui.execute_action("new"));yield delay(.7)
    player=unreal.GameplayStatics.get_player_pawn(w,0)
    companion=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    player.set_actor_location(companion.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    gameplay=player.get_component_by_class(unreal.HearthwardGameplayComponent)
    ai=sub(unreal.HearthwardLocalAISubsystem,w);save=sub(unreal.HearthwardSaveSubsystem,w)

    p=profile(ai,"initial")
    check("initial_no_prior",not p.stable and p.samples==0)

    check("direct_follow_1",gameplay.order_companion("follow"));yield delay(.05)
    check("direct_follow_2",gameplay.order_companion("follow"));yield delay(.05)
    p=profile(ai,"after_two_direct_follow")
    check("two_samples_not_stable",not p.stable and p.follow_count==2)

    # Same high-level choice through the candidate/confirmation boundary also counts as observed cooperation.
    check("stage_confirmed_follow",ai.set_structured_goal(player,companion,directive_goal("follow")))
    check("confirm_follow",ai.confirm_candidate(ai.get_candidate_id()));yield delay(.05)
    p=profile(ai,"after_confirmed_follow")
    check("three_follow_establish_prior",p.stable and str(p.preferred_directive).lower()=="follow" and p.follow_count==3)

    # Explicitly choose wait once. The learned prior may remain, but it must not override the new world state.
    check("explicit_wait",gameplay.order_companion("wait"));yield delay(.1)
    check("world_is_wait",str(gameplay.companion_order).lower()=="wait")
    p=profile(ai,"after_one_hold")
    check("one_contradiction_does_not_instantly_flip",p.stable and str(p.preferred_directive).lower()=="follow")
    before_order=str(gameplay.companion_order).lower()
    check("refresh_suggestions",ai.refresh_suggestions(player,companion))
    suggestions=ai.get_suggestions()
    report["suggestions_after_prior"]=[{"kind":str(x.kind),"label":x.label,"message":x.message} for x in suggestions]
    check("coordination_suggestion_present",any(str(x.kind).lower()=="coordination" and x.message=="跟着我。" for x in suggestions))
    check("refresh_does_not_apply_prior",str(gameplay.companion_order).lower()==before_order=="wait")

    # Persist the event-derived profile.
    saved=save_point(save)
    saved_profile=profile(ai,"saved_follow_prior")

    # Sustained new behavior adapts the rolling prior to assist.
    for i in range(6):
        check("assist_"+str(i),gameplay.order_companion("attack"));yield delay(.03)
    p=profile(ai,"after_sustained_assist")
    check("prior_adapts_to_assist",p.stable and str(p.preferred_directive).lower()=="assist" and p.assist_count>=6)

    # Restore the saved world/memory boundary; profile is reconstructed from restored directive events.
    check("load_profile",save.load_point(saved));yield delay(.5)
    player=unreal.GameplayStatics.get_player_pawn(w,0)
    companion=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    player.set_actor_location(companion.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    p=profile(ai,"restored_follow_prior")
    check("restored_profile_matches_saved",p.stable and str(p.preferred_directive).lower()=="follow"
          and p.samples==saved_profile.samples and p.follow_count==saved_profile.follow_count and p.hold_count==saved_profile.hold_count)
    check("restored_suggestions",ai.refresh_suggestions(player,companion))
    check("restored_follow_suggestion",any(str(x.kind).lower()=="coordination" and x.message=="跟着我。" for x in ai.get_suggestions()))

    reasons=[str(e.reason) for e in ai.get_events() if str(e.kind).lower()=="directive"]
    report["directive_sources"]=reasons
    check("records_direct_and_confirmed_sources","direct_control" in reasons and "dialogue_confirm" in reasons)

    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error: report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()

flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>120: raise TimeoutError("TASK-037 coordination prior PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
