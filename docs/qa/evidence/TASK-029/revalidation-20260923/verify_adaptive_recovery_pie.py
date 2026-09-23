"""TASK-032 PIE: move an active collection source and require deterministic adaptive replanning without manual retry."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task032";out.mkdir(parents=True,exist_ok=True)
result_path=out/"adaptive-recovery-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}
st={}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)

def wait(predicate,seconds=40): return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def subsystem(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def sample(c,label):
    report["samples"].append({
        "label":label,
        "action":c.get_execution_action(),
        "phase":str(c.get_phase()),
        "block":c.block_reason,
        "acquired":c.get_acquired(),
        "delivered":c.get_delivered(),
        "carried":c.get_carried(),
        "location":str(c.get_actor_location())
    })

def collect_goal(n):
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
    ai=subsystem(unreal.HearthwardLocalAISubsystem,w);store=subsystem(unreal.HearthwardStorageSubsystem,w)
    st["c"]=c

    p.set_actor_location(unreal.Vector(-120,-200,100),False,True)
    c.set_actor_location(unreal.Vector(-200,100,100),False,True)
    c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    source_actor=c.source.get_owner()
    source_actor.set_actor_location(unreal.Vector(400,100,100),False,True)
    existing=c.bag.get_item_count("wood")
    if existing: c.bag.try_remove("wood",existing)
    source_existing=c.source.get_item_count("wood")
    if source_existing: c.source.try_remove("wood",source_existing)
    c.source.try_add("wood",12)
    yield delay(.3)

    camp_before=store.get_item_count("wood")
    source_before=c.source.get_item_count("wood")
    check("stage_collect",ai.set_structured_goal(p,c,collect_goal(2)))
    check("confirm_collect",ai.confirm_candidate(ai.get_candidate_id()))
    check("starts_typed_executor",c.get_execution_action()=="MoveTo:Source")
    yield wait(lambda:c.get_execution_action()=="Gather:Source",25)
    sample(c,"gather_before_source_move")

    # Move the same authoritative source while the five-second gather action is active.
    source_actor.set_actor_location(unreal.Vector(850,220,100),False,True)
    yield delay(.15)
    sample(c,"after_source_move")
    check("enters_adaptive_replan",c.get_execution_action().startswith("Recovery:Replan"))
    check("replan_reason_visible","SOURCE_POSITION_CHANGED" in c.block_reason)
    check("no_fake_progress_during_replan",c.get_acquired()==0 and c.get_delivered()==0)

    yield wait(lambda:c.get_execution_action()=="MoveTo:Source",5)
    sample(c,"replan_released")
    yield wait(lambda:c.get_execution_action()=="Gather:Source",25)
    sample(c,"gather_after_reposition")
    yield wait(lambda:c.get_phase()==unreal.HearthwardCompanionPhase.COMPLETED,45)
    sample(c,"completed")

    check("completes_without_manual_resume",c.get_delivered()==2 and c.get_acquired()==2 and c.get_carried()==0)
    check("real_source_debited",c.source.get_item_count("wood")==source_before-2)
    check("real_camp_credit",store.get_item_count("wood")==camp_before+2)
    gameplay=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    check("recovery_status_cleared",c.get_execution_action()=="None" and "REPLANNING" not in c.block_reason)
    check("routine_resumed_after_recovery",gameplay.is_companion_routine_enabled()
          and str(gameplay.get_companion_tactical_intent()).lower()=="routine"
          and c.block_reason=="自由活动 · "+str(gameplay.get_companion_routine_activity()))

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
        if time.monotonic()-started>120: raise TimeoutError("TASK-032 adaptive recovery PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:
                    c=st.get("c")
                    raise TimeoutError("wait expired action="+(c.get_execution_action() if c else "startup")
                        +" phase="+(str(c.get_phase()) if c else "none")
                        +" block="+(c.block_reason if c else "none"))
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
