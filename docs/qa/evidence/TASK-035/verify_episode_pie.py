"""TASK-035 PIE: real command events project into grounded episodes and survive save/load without copied summaries."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task035";out.mkdir(parents=True,exist_ok=True)
result_path=out/"episode-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=40): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def goal():
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":"collect","item":"wood","quantity":2,"quantity_mode":"additional_acquired","source_ref":"S1"}.items():
        g.set_editor_property(k,v)
    return g
def save_point(save):
    before={x.save_id.to_string() for x in save.get_points()}
    check("save_episode_world",save.save_point(True))
    created=[x for x in save.get_points() if x.save_id.to_string() not in before]
    check("save_episode_new_id",len(created)==1)
    return created[0].save_id
def episode_dict(e):
    return {
        "command":e.command.to_string(),"item":str(e.item),"acquired":int(e.acquired),"delivered":int(e.delivered),
        "crafted":int(e.crafted),"repaired":int(e.repaired),"replans":int(e.replans),"completed":bool(e.completed),
        "cancelled":bool(e.cancelled),"reasons":[str(x) for x in e.reasons],
        "evidence":[x.to_string() for x in e.evidence]
    }

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);ui=pc.get_hud().screen
    check("new_campaign",ui.execute_action("new"));yield delay(.7)
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    ai=sub(unreal.HearthwardLocalAISubsystem,w);save=sub(unreal.HearthwardSaveSubsystem,w)
    source_actor=c.source.get_owner()

    c.set_actor_location(unreal.Vector(-200,100,100),False,True);c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    p.set_actor_location(unreal.Vector(-320,100,100),False,True);source_actor.set_actor_location(unreal.Vector(420,100,100),False,True)
    existing=c.bag.get_item_count("wood")
    if existing:c.bag.try_remove("wood",existing)
    src=c.source.get_item_count("wood")
    if src:c.source.try_remove("wood",src)
    c.source.try_add("wood",12);yield delay(.3)

    check("candidate",ai.set_structured_goal(p,c,goal()))
    check("confirm",ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda:c.get_execution_action()=="Gather:Source",25)
    source_actor.set_actor_location(unreal.Vector(820,220,100),False,True)
    yield wait(lambda:c.get_execution_action().startswith("Recovery:Replan"),4)
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED,50)
    yield delay(.3)

    episodes=ai.get_recent_episodes();check("episode_created",len(episodes)>=1)
    e=episodes[0];report["samples"].append({"before_save":episode_dict(e)})
    check("episode_command_matches",e.command.to_string()==c.get_command_id().to_string())
    check("episode_real_effects",e.acquired==2 and e.delivered==2 and e.replans==1 and e.completed)
    check("episode_reason", "SOURCE_POSITION_CHANGED" in [str(x) for x in e.reasons])
    check("episode_has_evidence",len(e.evidence)>=4)

    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    check("query_grounded_history",ai.query_recent_history(p,c,"wood"))
    line=ai.get_npc_line();report["history_before_save"]=line
    check("history_mentions_real_counts","实际取得2" in line and "实际入库2" in line)
    check("history_mentions_replan","重规划1次" in line and "SOURCE_POSITION_CHANGED" in line)
    check("history_cites_evidence","证据：" in line)

    saved=save_point(save)
    command=e.command.to_string();evidence=[x.to_string() for x in e.evidence]
    check("load_episode_world",save.load_point(saved));yield delay(.5)
    restored=ai.get_recent_episodes();check("episode_restored_from_events",len(restored)>=1)
    re=restored[0];report["samples"].append({"after_load":episode_dict(re)})
    check("episode_identity_stable",re.command.to_string()==command)
    check("episode_evidence_stable",[x.to_string() for x in re.evidence]==evidence)
    check("episode_projection_stable",re.acquired==2 and re.delivered==2 and re.replans==1 and re.completed)
    p=unreal.GameplayStatics.get_player_pawn(w,0);c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    check("query_after_restore",ai.query_recent_history(p,c,"wood"))
    check("grounded_history_after_restore",ai.get_npc_line()==line)

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
        if time.monotonic()-started>150: raise TimeoutError("TASK-035 episode PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
