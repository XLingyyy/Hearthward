"""TASK-038 PIE: camp routine is visible, real-nav, explicitly suppressible, task-preemptible, and save-stable."""
import json,time,traceback,math
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task038";out.mkdir(parents=True,exist_ok=True)
result_path=out/"camp-routine-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=35): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def dist(a,b): return math.hypot(a.x-b.x,a.y-b.y)
def snap(g,c,label):
    report["samples"].append({
        "label":label,
        "routine_enabled":bool(g.is_companion_routine_enabled()),
        "activity":str(g.get_companion_routine_activity()),
        "order":str(g.companion_order),
        "tactical":str(g.get_companion_tactical_intent()),
        "reason":g.get_companion_combat_reason(),
        "execution":c.get_execution_action(),
        "phase":str(c.get_phase()),
        "location":str(c.get_actor_location()),
        "block":c.block_reason
    })
def order_goal(item):
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":"companion_order","item":item,"quantity":1,"quantity_mode":"directive","source_ref":"player"}.items():
        g.set_editor_property(k,v)
    return g
def collect_goal():
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":"collect","item":"wood","quantity":1,"quantity_mode":"additional_acquired","source_ref":"S1"}.items():
        g.set_editor_property(k,v)
    return g
def save_point(save):
    before={x.save_id.to_string() for x in save.get_points()}
    check("save_routine_state",save.save_point(True))
    made=[x for x in save.get_points() if x.save_id.to_string() not in before]
    check("save_routine_new_id",len(made)==1)
    return made[0].save_id

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
    g=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    ai=sub(unreal.HearthwardLocalAISubsystem,w);save=sub(unreal.HearthwardSaveSubsystem,w)
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)

    check("routine_enabled_by_new_progress",g.is_companion_routine_enabled())
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="routine",4)
    snap(g,c,"routine_initial")
    start=c.get_actor_location()
    yield wait(lambda:dist(c.get_actor_location(),start)>25,8)
    moved=dist(c.get_actor_location(),start);report["routine_initial_move_cm"]=moved
    check("routine_uses_real_movement",moved>25)
    check("routine_activity_visible",str(g.get_companion_routine_activity()).lower() not in ("none",""))

    # Explicit wait owns the companion and permanently suppresses autonomous routine until re-enabled.
    check("explicit_wait",g.order_companion("wait"));yield delay(.25)
    snap(g,c,"after_explicit_wait")
    check("wait_disables_routine",not g.is_companion_routine_enabled())
    check("wait_is_hold",str(g.get_companion_tactical_intent()).lower()=="hold")
    still=c.get_actor_location();yield delay(.8)
    check("wait_stays_still",dist(c.get_actor_location(),still)<12)

    # Re-enable only through an explicit confirmed high-level routine directive.
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    check("stage_routine",ai.set_structured_goal(p,c,order_goal("routine")))
    check("confirm_routine",ai.confirm_candidate(ai.get_candidate_id()));yield delay(.2)
    check("routine_reenabled",g.is_companion_routine_enabled() and str(g.companion_order).lower()=="wait")
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="routine",4)
    snap(g,c,"routine_reenabled")

    saved=save_point(save)

    # Typed task takes navigation ownership without revoking the player's routine authorization.
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    check("stage_collect",ai.set_structured_goal(p,c,collect_goal()))
    check("confirm_collect",ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda:c.get_execution_action()=="MoveTo:Source",4)
    yield delay(.1)
    snap(g,c,"typed_task_preempts_routine")
    check("routine_authorization_preserved",g.is_companion_routine_enabled())
    check("task_owns_navigation",g.get_companion_combat_reason()=="TASK_OWNS_COMPANION")
    check("routine_activity_suspended",str(g.get_companion_routine_activity()).lower() in ("none",""))
    check("cancel_task",ai.cancel_execution(p,c));yield delay(.3)
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="routine",4)
    check("routine_resumes_after_task_cancel",g.is_companion_routine_enabled())
    snap(g,c,"routine_after_cancel")

    # Explicit follow takes control and disables routine.
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    check("explicit_follow",g.order_companion("follow"));yield delay(.2)
    check("follow_disables_routine",not g.is_companion_routine_enabled())
    check("follow_tactical",str(g.get_companion_tactical_intent()).lower()=="follow")
    snap(g,c,"follow_override")

    # Loading the saved routine-authorized boundary restores both the order and authorization.
    check("load_routine_state",save.load_point(saved));yield delay(.5)
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    check("routine_authorization_restored",g.is_companion_routine_enabled() and str(g.companion_order).lower()=="wait")
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="routine",4)
    snap(g,c,"routine_after_load")
    check("routine_activity_restored",str(g.get_companion_routine_activity()).lower() not in ("none",""))

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
        if time.monotonic()-started>140: raise TimeoutError("TASK-038 routine PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
