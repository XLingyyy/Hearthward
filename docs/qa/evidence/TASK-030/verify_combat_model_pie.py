"""TASK-030 real-model smoke: natural language -> companion_order card -> confirm -> UE tactics."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task030";out.mkdir(parents=True,exist_ok=True)
result=out/"combat-model-pie-results.json";report={"ok":False,"checks":{},"cases":[]}

def check(n,v):
    report["checks"][n]=bool(v)
    if not v: raise AssertionError(n)
def wait(p,s=150): return p,time.monotonic()+s
def delay(s):
    end=time.monotonic()+s
    return wait(lambda:time.monotonic()>=end,s+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)

def send(ai,p,c,text):
    check("send_"+str(len(report["cases"])),ai.submit_player_text(p,c,text))
    return wait(lambda:not ai.is_busy(),150)

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check("new_campaign",ui.execute_action("new"));yield delay(.5)
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    ai=sub(unreal.HearthwardLocalAISubsystem,w);gameplay=p.get_component_by_class(unreal.HearthwardGameplayComponent)

    yield send(ai,p,c,"跟着我。")
    raw=ai.get_last_structured_result();report["cases"].append({"input":"跟着我。","raw":raw,"status":ai.get_status()})
    check("follow_real_model_candidate",ai.has_candidate())
    data=json.loads(raw)
    check("follow_real_model_contract",data.get("intent")=="companion_order" and data.get("item")=="follow")
    check("follow_world_unchanged_before_confirm",str(gameplay.companion_order).lower()=="wait")
    check("follow_confirm",ai.confirm_candidate(ai.get_candidate_id()))
    check("follow_applied",str(gameplay.companion_order).lower()=="follow")

    yield send(ai,p,c,"帮我对付附近的威胁。")
    raw=ai.get_last_structured_result();report["cases"].append({"input":"帮我对付附近的威胁。","raw":raw,"status":ai.get_status()})
    check("assist_real_model_candidate",ai.has_candidate())
    data=json.loads(raw)
    check("assist_real_model_contract",data.get("intent")=="companion_order" and data.get("item")=="assist")
    check("assist_confirm",ai.confirm_candidate(ai.get_candidate_id()))
    yield delay(.5)
    check("assist_applied",str(gameplay.companion_order).lower()=="attack")
    check("ue_selects_target_after_model_strategy",str(gameplay.get_companion_combat_target()).lower()=="guard_1")
    check("filtered_context_has_combat_state","companion_combat" in ai.get_last_filtered_context())

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
        if time.monotonic()-started>330: raise TimeoutError("TASK-030 real model timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
