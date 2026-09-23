"""TASK-040 R1/R2 PIE: real Qwen uses bounded context tiers and reaches routine consistently."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
root=Path(unreal.Paths.project_dir())
out=root/"Saved/Task040";out.mkdir(parents=True,exist_ok=True)
result_path=out/"context-budget-pie-results.json"
jsonl_path=out/"model-results.jsonl"
report={"ok":False,"checks":{},"cases":[]}
ITEMS=['wood','stone','ore','meat','arrow','axe','bow','hood','armor','gloves','boots','belt','shield','quiver','amulet','roast','herb','hide','rope','flower','medicine','firepot']

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=180): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def diag(ai,label,text):
    raw=ai.get_last_structured_result()
    row={
        "label":label,"input":text,"raw":raw,"status":ai.get_status(),"line":ai.get_npc_line(),
        "reason":ai.get_reason_code(),"tokens":int(ai.get_input_tokens()),
        "tier":ai.get_context_tier(),"generation_calls":int(ai.get_generation_calls()),
        "dropped":[str(x) for x in ai.get_dropped_context_fields()],
        "candidate":bool(ai.has_candidate())
    }
    report["cases"].append(row)
    with jsonl_path.open("a",encoding="utf-8") as f:f.write(json.dumps(row,ensure_ascii=False)+"\n")
    return row
def send(ai,p,c,label,text,timeout=180):
    check(label+"_submitted",ai.submit_player_text(p,c,text))
    return wait(lambda:not ai.is_busy(),timeout)

def run():
    if jsonl_path.exists():jsonl_path.unlink()
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor,30);yield delay(1)
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
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    yield delay(.2)
    ai=sub(unreal.HearthwardLocalAISubsystem,w)
    gameplay=p.get_component_by_class(unreal.HearthwardGameplayComponent)

    # CTX-01 / M01 clean request. Do not confirm: this verifies no world effect before confirmation.
    text="请去新采四份木材并带回仓库"
    yield send(ai,p,c,"clean_collect",text)
    row=diag(ai,"clean_collect",text)
    check("clean_not_overflow",row["reason"]!="CONTEXT_OVERFLOW")
    check("clean_real_count_within_budget",0<row["tokens"]<=3328)
    check("clean_exactly_one_generation",row["generation_calls"]==1)
    data=json.loads(row["raw"])
    check("clean_collect_contract",data.get("intent")=="collect" and data.get("item")=="wood" and int(data.get("quantity",0))==4)
    check("clean_candidate_waits",ai.has_candidate() and c.get_requested()==0)

    # CAP-03 / M07: first explicitly hold so routine is disabled, then prove the model card does not re-enable it before confirmation.
    check("routine_precondition_hold",gameplay.order_companion("wait"))
    check("routine_precondition_disabled",not bool(gameplay.is_companion_routine_enabled()))
    text="恢复营地自由活动"
    yield send(ai,p,c,"routine",text)
    row=diag(ai,"routine",text)
    check("routine_not_overflow",row["reason"]!="CONTEXT_OVERFLOW")
    check("routine_budget",0<row["tokens"]<=3328)
    check("routine_one_generation",row["generation_calls"]==1)
    data=json.loads(row["raw"])
    check("routine_raw_model_contract",data.get("intent")=="companion_order" and data.get("item")=="routine")
    check("routine_candidate_before_confirm",ai.has_candidate())
    before=bool(gameplay.is_companion_routine_enabled())
    check("routine_confirm",ai.confirm_candidate(ai.get_candidate_id()))
    yield delay(.1)
    check("routine_only_changes_after_confirm",(not before) and bool(gameplay.is_companion_routine_enabled()))

    # CTX-02 pressure state: 64 active player records (4 agreements), 22 beliefs, 128 persisted directive events.
    # Move away so player reports are not immediately replaced by firsthand observation.
    away=c.camp.get_actor_location()+unreal.Vector(1200,0,0)
    c.set_actor_location(away,False,True);p.set_actor_location(away+unreal.Vector(-120,0,0),False,True)
    yield delay(.2)

    for i in range(60):
        check("pressure_claim_%02d"%i,ai.put_player_memory(p,c,unreal.Guid(),"claim","压力测试无关记录%02d：只用于上下文预算。"%i))
    for i in range(4):
        check("pressure_agreement_%d"%i,ai.put_player_memory(p,c,unreal.Guid(),"agreement","压力测试约定%d：回答时保持简短。"%i))
    for i,item in enumerate(ITEMS):
        check("pressure_belief_"+item,ai.report_camp_inventory(p,c,item,i+1))
    for i in range(128):
        order=("wait","follow","attack")[i%3]
        check("pressure_directive_%03d"%i,gameplay.order_companion(order))
    yield delay(.2)
    check("pressure_event_count",len(ai.get_events())==128)

    text="请去新采四份木材并带回仓库"
    yield send(ai,p,c,"pressure_collect",text)
    row=diag(ai,"pressure_collect",text)
    check("pressure_not_overflow",row["reason"]!="CONTEXT_OVERFLOW")
    check("pressure_real_count_within_budget",0<row["tokens"]<=3328)
    check("pressure_exactly_one_generation",row["generation_calls"]==1)
    check("pressure_bounded_tier",row["tier"] in ("full_relevant","compact_relevant","required_minimal"))
    data=json.loads(row["raw"])
    check("pressure_collect_contract",data.get("intent")=="collect" and data.get("item")=="wood" and int(data.get("quantity",0))==4)
    check("pressure_candidate_waits",ai.has_candidate() and c.get_requested()==0)

    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error:report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    try:unreal.SystemLibrary.quit_editor()
    except Exception:pass

flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>600:raise TimeoutError("TASK-040 context/model PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
