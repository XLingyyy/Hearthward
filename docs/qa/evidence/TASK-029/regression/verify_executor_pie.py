"""TASK-028 deterministic PIE: typed-plan executor transitions and save reconstruction."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/"Saved/Task028";out.mkdir(parents=True,exist_ok=True)
result_path=out/"executor-pie-results.json"
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
P=unreal.HearthwardCompanionPhase
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
def action(comp,label):
    value=comp.get_execution_action()
    report["samples"].append({"label":label,"action":value,"phase":str(comp.get_phase())})
    return value

def goal(intent,item,quantity,mode,source):
    g=unreal.HearthwardAgentGoal()
    for k,v in {"intent":intent,"item":item,"quantity":quantity,"quantity_mode":mode,"source_ref":source}.items():
        g.set_editor_property(k,v)
    return g

def start(ai,p,c,g,name):
    check(name+"_card",ai.set_structured_goal(p,c,g))
    check(name+"_confirm",ai.confirm_candidate(ai.get_candidate_id()))

def save_point(save,name):
    before={x.save_id.to_string() for x in save.get_points()}
    check(name,save.save_point(True))
    created=[x for x in save.get_points() if x.save_id.to_string() not in before]
    check(name+"_new_id",len(created)==1)
    return created[0].save_id

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0)
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    gameplay=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    if not gameplay.enabled:
        gameplay.enable_adventure()
    save=subsystem(unreal.HearthwardSaveSubsystem,w)
    if save.enable_prototype():
        raise AssertionError("prototype unexpectedly enabled without explicit fixture")
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.CreateTest",pc)
    yield delay(.5)
    actors=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)
    if len(actors)!=1:
        raise AssertionError("explicit companion fixture was not created")
    c=actors[0]
    if not save.enable_prototype():
        raise AssertionError("prototype did not enable after explicit fixture")
    check("new_campaign",save.start_new_progress());yield delay(.5)
    ai=subsystem(unreal.HearthwardLocalAISubsystem,w);store=subsystem(unreal.HearthwardStorageSubsystem,w)
    build=p.get_component_by_class(unreal.HearthwardBuildingComponent);bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    st.update(c=c,ai=ai,save=save,store=store)

    # Create one real workbench and a stable fixture snapshot.
    p.set_actor_location(unreal.Vector(-200,-200,100),False,True);pc.set_control_rotation(unreal.Rotator(pitch=-20,yaw=0))
    bag.try_add("wood",8);check("select_workbench",build.select_building("workbench"));yield delay(.3)
    check("build_workbench",build.confirm_placement());yield wait(lambda:build.building_count()==1,12)
    p.set_actor_location(unreal.Vector(-110,-200,100),False,True)
    c.set_actor_location(unreal.Vector(-200,100,100),False,True);c.camp.set_actor_location(unreal.Vector(-200,100,100),False,True)
    c.source.get_owner().set_actor_location(unreal.Vector(400,100,100),False,True)
    c.bag.try_add("wood",12);c.bag.try_add("rope",4);c.bag.try_add("axe",1);c.set_editor_property("owned_durability",{"axe":20.0})
    c.source.try_add("wood",40);yield delay(.5)
    base=save_point(save,"baseline_save")

    # collect: unrelated bag cargo is returned first; same-item retained cargo is tested by the legacy companion suite.
    existing_target=c.bag.get_item_count("wood")
    if existing_target: c.bag.try_remove("wood",existing_target)
    collect_store_before=store.get_item_count("wood")
    collect_bag_wood_before=c.bag.get_item_count("wood")
    collect_source_before=c.source.get_item_count("wood")
    start(ai,p,c,goal("collect","wood",6,"additional_acquired","S1"),"collect")
    check("collect_existing_cargo_returns_first",action(c,"collect_start")=="MoveTo:Camp")
    yield wait(lambda:c.get_execution_action()=="MoveTo:Source",20)
    check("existing_cargo_not_counted_as_goal_progress",c.get_acquired()==0 and c.get_delivered()==0)
    yield wait(lambda:c.get_execution_action()=="Gather:Source",20)
    check("collect_enters_gather",c.get_phase()==P.GATHERING)
    yield wait(lambda:c.get_acquired()>=4,20)
    check("collect_moves_to_camp_after_real_acquire",action(c,"collect_after_acquire")=="MoveTo:Camp")
    acquired=c.get_acquired();carried=c.get_carried();source_before=c.source.get_item_count("wood");camp_before=store.get_item_count("wood")
    partial=save_point(save,"partial_save")
    check("partial_load",save.load_point(partial));yield delay(.3)
    check("plan_reconstructed_from_phase",action(c,"collect_after_load")=="MoveTo:Camp")
    check("load_preserves_progress",c.get_acquired()==acquired and c.get_carried()==carried and c.source.get_item_count("wood")==source_before and store.get_item_count("wood")==camp_before)
    yield wait(lambda:c.get_phase()==P.COMPLETED,45)
    collect_expected_store=collect_store_before+collect_bag_wood_before+6
    check("collect_exact_after_restore",c.get_acquired()==6 and c.get_delivered()==6 and c.get_carried()==0
          and store.get_item_count("wood")==collect_expected_store and c.source.get_item_count("wood")==collect_source_before-6)
    yield delay(.5);check("collect_no_duplicate_after_completion",store.get_item_count("wood")==collect_expected_store)

    # finite-source shortage: v2 additional_acquired semantics stop at the real six units and wait at camp.
    check("reset_for_shortage",save.load_point(base));yield delay(.3)
    for item in ["wood","rope","axe"]:
        count=c.bag.get_item_count(item)
        if count: c.bag.try_remove(item,count)
    source_count=c.source.get_item_count("wood")
    if source_count: c.source.try_remove("wood",source_count)
    c.source.try_add("wood",6)
    shortage_store_before=store.get_item_count("wood")
    start(ai,p,c,goal("collect","wood",8,"additional_acquired","S1"),"shortage")
    check("shortage_starts_at_source",action(c,"shortage_start")=="MoveTo:Source")
    check("shortage_starts_without_progress",c.get_acquired()==0 and c.get_delivered()==0)
    yield wait(lambda:c.get_phase()==P.WAITING_AT_CAMP,90)
    check("shortage_only_delivers_real_units",c.get_acquired()==6 and c.get_delivered()==6 and c.get_carried()==0)
    check("shortage_conserves_inventory",c.source.get_item_count("wood")==0 and store.get_item_count("wood")==shortage_store_before+6)
    check("shortage_waits_on_same_plan",c.get_execution_action()=="MoveTo:Source" and bool(c.block_reason))

    # cancelled command retains real cargo. A replacement goal may adopt only the requested amount;
    # surplus physical cargo remains in the NPC bag and is not attributed to the new command.
    check("reset_for_retained",save.load_point(base));yield delay(.3)
    for item in ["wood","rope","axe"]:
        retained_existing=c.bag.get_item_count(item)
        if retained_existing: c.bag.try_remove(item,retained_existing)
    start(ai,p,c,goal("collect","wood",8,"additional_acquired","S1"),"retained_seed")
    yield wait(lambda:c.get_phase()==P.RETURNING and c.get_carried()==4,45)
    check("retained_seed_has_real_four",c.bag.get_item_count("wood")==4 and c.get_carried()==4 and c.get_delivered()==0)
    check("retained_cancel",c.cancel(p))
    retained_store_before=store.get_item_count("wood")
    start(ai,p,c,goal("collect","wood",1,"additional_acquired","S1"),"retained_replacement")
    check("retained_replacement_returns_first",action(c,"retained_replacement_start")=="MoveTo:Camp")
    yield wait(lambda:c.get_phase()==P.COMPLETED,25)
    check("retained_only_requested_unit_delivered",c.get_delivered()==1 and store.get_item_count("wood")==retained_store_before+1)
    check("retained_surplus_stays_physical",c.bag.get_item_count("wood")==3)

    # bag craft: same executor, different plan, output returns to camp.
    check("reset_for_craft",save.load_point(base));yield delay(.3)
    start(ai,p,c,goal("craft","arrows",2,"batches","bag"),"craft")
    check("craft_starts_move_workshop",action(c,"craft_start")=="MoveTo:Workshop")
    yield wait(lambda:c.get_execution_action()=="CommitWorkshop:Workshop",20)
    yield wait(lambda:c.get_acquired()==8,20)
    check("craft_commit_advances_to_return",action(c,"craft_after_commit")=="MoveTo:Camp")
    check("craft_output_real_before_deposit",c.bag.get_item_count("arrow")==8 and store.get_item_count("arrow")==0)
    yield wait(lambda:c.get_phase()==P.COMPLETED,30)
    check("craft_exact_delivery",c.get_delivered()==8 and c.bag.get_item_count("arrow")==0 and store.get_item_count("arrow")==8)

    # bag repair: no camp deposit action; completion is still receipt-bound.
    check("reset_for_repair",save.load_point(base));yield delay(.3)
    start(ai,p,c,goal("repair","axe",1,"one_owned","bag"),"repair")
    check("repair_starts_move_workshop",action(c,"repair_start")=="MoveTo:Workshop")
    yield wait(lambda:c.get_phase()==P.COMPLETED,30)
    check("repair_completed_without_deposit",c.owned_durability.get("axe")==100 and c.get_delivered()==1 and c.get_execution_action()=="None")
    check("repair_material_cost",c.bag.get_item_count("wood")==10 and c.bag.get_item_count("rope")==3)

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
        if time.monotonic()-started>180: raise TimeoutError("TASK-028 executor PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:
                    c=st.get("c")
                    raise TimeoutError("wait expired action="+(c.get_execution_action() if c else "startup")
                        +" phase="+(str(c.get_phase()) if c else "none")
                        +" delivered="+(str(c.get_delivered()) if c else "none")
                        +" acquired="+(str(c.get_acquired()) if c else "none")
                        +" carried="+(str(c.get_carried()) if c else "none")
                        +" block="+(c.block_reason if c else "none"))
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
