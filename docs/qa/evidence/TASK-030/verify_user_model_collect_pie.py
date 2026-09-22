"""Exact user-reported path: real Qwen text -> collect card -> confirm -> physical movement -> delivery."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task030";out.mkdir(parents=True,exist_ok=True)
result=out/"user-model-collect-pie-results.json";report={"ok":False,"checks":{}}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=150): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check("new_campaign",ui.execute_action("new"));yield delay(.5)
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True);yield delay(.2)
    ai=sub(unreal.HearthwardLocalAISubsystem,w)
    start=c.get_actor_location()

    text="帮我采集两份木材带回营地。"
    check("real_model_submit",ai.submit_player_text(p,c,text))
    yield wait(lambda:not ai.is_busy(),150)
    raw=ai.get_last_structured_result();report["raw"]=raw
    data=json.loads(raw)
    check("model_collect_intent",data.get("intent")=="collect")
    check("model_wood",data.get("item")=="wood")
    check("model_quantity_two",int(data.get("quantity",0))==2)
    check("candidate_waits_for_confirm",ai.has_candidate())
    check("world_unchanged_before_confirm",c.get_requested()==0)

    check("confirm",ai.confirm_candidate(ai.get_candidate_id()))
    check("executor_starts",c.get_requested()==2 and c.get_execution_action() in ["MoveTo:Source","MoveTo:Camp"])
    def moved_cm():
        d=c.get_actor_location()-start
        return (d.x*d.x+d.y*d.y)**0.5
    yield wait(lambda:moved_cm()>50,15)
    report["moved_cm"]=moved_cm()
    check("physical_movement",report["moved_cm"]>50)
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED,90)
    check("delivered_two",c.get_delivered()==2 and c.get_carried()==0)

    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error:report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    try:unreal.SystemLibrary.quit_editor()
    except Exception:pass

flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>260:raise TimeoutError("real model collect retest timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
