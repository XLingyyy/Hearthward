"""TASK-029 post-main-merge runtime smoke.

Uses the explicit development-only prototype fixture instead of the production "new game"
action, which now correctly travels to the Natural World.
"""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task029"
out.mkdir(parents=True,exist_ok=True)
result_path=out/"post-merge-runtime-smoke.json"
report={"ok":False,"checks":{},"samples":[]}
P=unreal.HearthwardCompanionPhase
state={}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value:
        raise AssertionError(name)

def wait(pred,seconds=40):
    return pred,time.monotonic()+seconds

def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)

def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()

def sub(cls):
    w=world()
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)

def companion():
    actors=unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.HearthwardCompanionFixture)
    return actors[0] if actors else None

def phase_is(value):
    c=companion()
    return bool(c) and c.get_phase()==value

def action_is(value):
    c=companion()
    return bool(c) and c.get_execution_action()==value

def goal_collect(quantity):
    g=unreal.HearthwardAgentGoal()
    for k,v in {
        "intent":"collect",
        "item":"wood",
        "quantity":quantity,
        "quantity_mode":"additional_acquired",
        "source_ref":"S1"
    }.items():
        g.set_editor_property(k,v)
    return g

def new_save_id(save,before):
    created=[x for x in save.get_points() if x.save_id.to_string() not in before]
    return created[0].save_id if len(created)==1 else None

def run():
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(1)

    w=world()
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    pc=unreal.GameplayStatics.get_player_controller(w,0)
    check("prototype_player_present",p is not None)

    save=sub(unreal.HearthwardSaveSubsystem)
    ai=sub(unreal.HearthwardLocalAISubsystem)
    store=sub(unreal.HearthwardStorageSubsystem)
    state.update(save=save)

    check("prototype_requires_explicit_fixture",not save.enable_prototype())
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.CreateTest",pc)
    yield delay(.5)
    c=companion()
    check("prototype_companion_created",c is not None)

    check("enable_prototype",save.enable_prototype())
    check("start_new_progress",save.start_new_progress())
    check("prototype_enabled",save.is_prototype_enabled())
    check("campaign_created",save.get_campaign_id().to_string()!="00000000000000000000000000000000")

    w=world();p=unreal.GameplayStatics.get_player_pawn(w,0);c=companion();ai=sub(unreal.HearthwardLocalAISubsystem);store=sub(unreal.HearthwardStorageSubsystem)
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-150,0,0),False,True)

    # TASK-029 suggestions: explicit refresh only; no cognition/world write from refresh.
    memory_before=ai.get_memory_revision()
    input_before=ai.get_last_input()
    context_before=ai.get_last_filtered_context()
    check("suggestions_start_empty",len(ai.get_suggestions())==0)
    check("suggestions_explicit_refresh",ai.refresh_suggestions(p,c))
    suggestions=ai.get_suggestions()
    check("suggestions_exactly_three",len(suggestions)==3)
    check("suggestions_no_memory_write",ai.get_memory_revision()==memory_before)
    check("suggestions_no_input_write",ai.get_last_input()==input_before)
    check("suggestions_no_context_write",ai.get_last_filtered_context()==context_before)
    check("suggestions_no_candidate",not ai.has_candidate())

    # Collect through deterministic candidate/confirm boundary.
    source_before=c.source.get_item_count("wood")
    if source_before<6:
        c.source.try_add("wood",6-source_before)
    store_before=store.get_item_count("wood")
    check("collect_stage",ai.set_structured_goal(p,c,goal_collect(2)))
    check("collect_confirm",ai.confirm_candidate(ai.get_candidate_id()))

    yield wait(lambda: companion() is not None and companion().get_acquired()>=1,45)
    c=companion();save=sub(unreal.HearthwardSaveSubsystem);store=sub(unreal.HearthwardStorageSubsystem)
    report["samples"].append({"stage":"pre_save","action":c.get_execution_action(),"acquired":c.get_acquired(),"carried":c.get_carried(),"delivered":c.get_delivered()})

    before={x.save_id.to_string() for x in save.get_points()}
    check("mid_task_save",save.save_point(True))
    sid=new_save_id(save,before)
    check("mid_task_save_has_id",sid is not None)
    snapshot=(c.get_acquired(),c.get_carried(),c.get_delivered(),c.source.get_item_count("wood"),store.get_item_count("wood"))

    check("mid_task_load",save.load_point(sid))
    yield delay(.3)
    c=companion();store=sub(unreal.HearthwardStorageSubsystem)
    check("restore_preserves_progress",(c.get_acquired(),c.get_carried(),c.get_delivered(),c.source.get_item_count("wood"),store.get_item_count("wood"))==snapshot)
    check("restore_rebuilds_execution",c.get_execution_action()!="None" and c.get_phase()!=P.COMPLETED)
    report["samples"].append({"stage":"post_load","action":c.get_execution_action(),"acquired":c.get_acquired(),"carried":c.get_carried(),"delivered":c.get_delivered()})

    yield wait(lambda: phase_is(P.COMPLETED),60)
    c=companion();store=sub(unreal.HearthwardStorageSubsystem)
    check("collect_completed_exactly",c.get_acquired()==2 and c.get_delivered()==2 and c.get_carried()==0)
    check("collect_world_settlement",store.get_item_count("wood")==store_before+2)
    report["samples"].append({"stage":"completed","action":c.get_execution_action(),"acquired":c.get_acquired(),"delivered":c.get_delivered(),"store":store.get_item_count("wood")})

    levels.editor_request_end_play()
    yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error:
        report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    unreal.SystemLibrary.quit_editor()

flow=run()
pending=None
started=time.monotonic()

def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>150:
            raise TimeoutError("TASK-029 post-merge runtime smoke timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:
                    c=companion()
                    raise TimeoutError("wait expired action="+(c.get_execution_action() if c else "none"))
                return
        pending=next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())

handle=unreal.register_slate_post_tick_callback(tick)
