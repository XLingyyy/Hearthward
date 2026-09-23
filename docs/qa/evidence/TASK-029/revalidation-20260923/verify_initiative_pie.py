"""TASK-034 PIE: event-driven initiatives surface without polling the model and respect communication range."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task034";out.mkdir(parents=True,exist_ok=True)
result_path=out/"initiative-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}
st={}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=40): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def goal(n=2):
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":"collect","item":"wood","quantity":n,"quantity_mode":"additional_acquired","source_ref":"S1"}.items():
        g.set_editor_property(k,v)
    return g
def start(ai,p,c,n,label):
    check(label+"_candidate",ai.set_structured_goal(p,c,goal(n)))
    check(label+"_confirm",ai.confirm_candidate(ai.get_candidate_id()))
def sample(ai,label):
    report["samples"].append({
        "label":label,
        "active":bool(ai.has_active_initiative()),
        "kind":str(ai.get_initiative_kind()),
        "initiative":ai.get_initiative_line(),
        "display_line":ai.get_npc_line(),
        "status":ai.get_status(),
        "busy":bool(ai.is_busy())
    })

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
    ai=sub(unreal.HearthwardLocalAISubsystem,w);st["c"]=c
    source_actor=c.source.get_owner()

    # Stable small scenario.
    c.set_actor_location(unreal.Vector(-200,100,100),False,True);c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    p.set_actor_location(unreal.Vector(-320,100,100),False,True);source_actor.set_actor_location(unreal.Vector(420,100,100),False,True)
    existing=c.bag.get_item_count("wood")
    if existing:c.bag.try_remove("wood",existing)
    src=c.source.get_item_count("wood")
    if src:c.source.try_remove("wood",src)
    c.source.try_add("wood",20);yield delay(.3)

    # Completion becomes proactive UI without another player prompt.
    start(ai,p,c,2,"complete")
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED,45)
    yield wait(ai.has_active_initiative,3)
    sample(ai,"completion_initiative")
    check("completion_kind",str(ai.get_initiative_kind()).lower().endswith("task_completed"))
    check("completion_line_grounded","2" in ai.get_initiative_line() and "木材" in ai.get_initiative_line())
    check("initiative_overrides_display",ai.get_status()=="弟弟主动提醒" and ai.get_npc_line()==ai.get_initiative_line())
    check("initiative_does_not_start_model",not ai.is_busy())

    # Let the first initiative expire, then prove a belief correction can initiate speech.
    yield wait(lambda:not ai.has_active_initiative(),12)
    away=c.camp.get_actor_location()+unreal.Vector(1000,0,0)
    c.set_actor_location(away,False,True);p.set_actor_location(away+unreal.Vector(-120,0,0),False,True);yield delay(.2)
    check("report_for_correction",ai.report_camp_inventory(p,c,"wood",99))
    c.set_actor_location(c.camp.get_actor_location(),False,True);p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    yield wait(ai.has_active_initiative,4)
    sample(ai,"belief_correction_initiative")
    check("correction_kind",str(ai.get_initiative_kind()).lower().endswith("belief_corrected"))
    check("correction_mentions_report", "99" in ai.get_initiative_line() and "亲自确认" in ai.get_initiative_line())

    yield wait(lambda:not ai.has_active_initiative(),12)

    # Replan occurs while player is out of communication range: no telepathic delivery.
    c.set_actor_location(c.camp.get_actor_location(),False,True);p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    source_actor.set_actor_location(unreal.Vector(420,100,100),False,True)
    start(ai,p,c,2,"replan")
    yield wait(lambda:c.get_execution_action()=="Gather:Source",25)
    far=c.get_actor_location()+unreal.Vector(5000,0,0);p.set_actor_location(far,False,True)
    source_actor.set_actor_location(unreal.Vector(820,220,100),False,True)
    yield wait(lambda:c.get_execution_action().startswith("Recovery:Replan"),4)
    yield delay(1)
    sample(ai,"replan_queued_player_far")
    check("no_remote_initiative",not ai.has_active_initiative())

    # Coming back into range releases the queued initiative, still without a model request.
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    yield wait(ai.has_active_initiative,4)
    sample(ai,"replan_delivered_when_near")
    check("replan_kind",str(ai.get_initiative_kind()).lower().endswith("task_replanned"))
    check("replan_line_grounded","重新调整路线" in ai.get_initiative_line())
    check("replan_no_model",not ai.is_busy())

    c.cancel(p)
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
        if time.monotonic()-started>150: raise TimeoutError("TASK-034 initiative PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:
                    c=st.get("c")
                    raise TimeoutError("wait expired action="+(c.get_execution_action() if c else "startup"))
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
