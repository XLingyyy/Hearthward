"""TASK-033 PIE: provenance-aware camp beliefs remain isolated from world truth while the NPC is away."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task033";out.mkdir(parents=True,exist_ok=True)
result_path=out/"belief-state-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}
st={}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=30): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w): return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)
def save_point(save,name):
    before={x.save_id.to_string() for x in save.get_points()}
    check(name,save.save_point(True))
    created=[x for x in save.get_points() if x.save_id.to_string() not in before]
    check(name+"_new_id",len(created)==1)
    return created[0].save_id

def sample(ai,item,label):
    b=ai.get_camp_stock_belief(item)
    report["samples"].append({
        "label":label,"known":bool(b.known),"value":int(b.value),
        "source":str(b.source),"recorded_at":float(b.recorded_at),"revision":int(b.revision),
        "line":ai.get_npc_line()
    })
    return b

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
    ai=sub(unreal.HearthwardLocalAISubsystem,w);store=sub(unreal.HearthwardStorageSubsystem,w);save=sub(unreal.HearthwardSaveSubsystem,w)
    bag=p.get_component_by_class(unreal.HearthwardInventoryComponent)
    st["c"]=c

    # First let the NPC establish a firsthand camp belief.
    c.set_actor_location(c.camp.get_actor_location(),False,True)
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    yield delay(.4)
    actual0=store.get_item_count("wood")
    b=sample(ai,"wood","initial_firsthand")
    check("firsthand_initial",b.known and b.value==actual0 and "FIRSTHAND" in str(b.source).upper())

    # Move away and replace that belief with an explicit player report. World truth must not change.
    away=c.camp.get_actor_location()+unreal.Vector(1000,0,0)
    c.set_actor_location(away,False,True);p.set_actor_location(away+unreal.Vector(-120,0,0),False,True)
    yield delay(.2)
    check("player_report_accepted",ai.report_camp_inventory(p,c,"wood",99))
    b=sample(ai,"wood","player_report")
    check("player_report_provenance",b.value==99 and "PLAYER_REPORT" in str(b.source).upper())
    check("player_report_does_not_change_world",store.get_item_count("wood")==actual0)
    check("query_report",ai.query_inventory(p,c,"wood"));yield delay(.05)
    check("query_labels_unconfirmed_report","99" in ai.get_npc_line() and "没有亲自确认" in ai.get_npc_line())

    # Mutate real storage while the NPC remains away. Belief must remain 99, not omnisciently follow world truth.
    bag.try_add("wood",1)
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Storage.CreateTestAccess",pc)
    access=unreal.GameplayStatics.get_all_actors_with_tag(w,"Hearthward.Storage.TestAccess")[0].get_component_by_class(unreal.HearthwardStorageAccessComponent)
    check("real_storage_mutated_while_away",access.transfer(bag,True,"wood",1,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch()).result==unreal.HearthwardInventoryResult.SUCCESS)
    actual1=store.get_item_count("wood");check("world_changed",actual1==actual0+1)
    yield delay(.4)
    b=sample(ai,"wood","away_after_world_change")
    check("away_belief_not_omniscient",b.value==99 and "PLAYER_REPORT" in str(b.source).upper())

    # Returning to camp creates stronger firsthand evidence and corrects the report.
    c.set_actor_location(c.camp.get_actor_location(),False,True)
    p.set_actor_location(c.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    yield delay(.4)
    b=sample(ai,"wood","returned_firsthand")
    check("firsthand_corrects_report",b.value==actual1 and "FIRSTHAND" in str(b.source).upper())
    check("query_firsthand",ai.query_inventory(p,c,"wood"));yield delay(.05)
    check("query_uses_confirmed_truth",str(actual1) in ai.get_npc_line() and "现在确认" in ai.get_npc_line())

    # Move away again; later world truth changes must not overwrite the last confirmed belief.
    away2=c.camp.get_actor_location()+unreal.Vector(1100,100,0)
    c.set_actor_location(away2,False,True);p.set_actor_location(away2+unreal.Vector(-120,0,0),False,True)
    yield delay(.2)
    bag.try_add("wood",1)
    check("second_world_mutation",access.transfer(bag,True,"wood",1,unreal.GuidLibrary.new_guid(),store.get_timeline_epoch()).result==unreal.HearthwardInventoryResult.SUCCESS)
    actual2=store.get_item_count("wood");check("world_changed_again",actual2==actual1+1)
    yield delay(.4)
    b=sample(ai,"wood","away_with_stale_firsthand")
    check("stale_firsthand_preserved",b.value==actual1 and "FIRSTHAND" in str(b.source).upper())
    check("query_stale_firsthand",ai.query_inventory(p,c,"wood"));yield delay(.05)
    check("query_admits_staleness",str(actual1) in ai.get_npc_line() and "不能保证" in ai.get_npc_line())

    # Beliefs share the same save boundary as the world and retain provenance across restore.
    saved=save_point(save,"belief_save")
    check("temporary_report_before_restore",ai.report_camp_inventory(p,c,"wood",77))
    temp=ai.get_camp_stock_belief("wood")
    check("temporary_report_visible",temp.value==77 and "PLAYER_REPORT" in str(temp.source).upper())
    check("belief_load",save.load_point(saved));yield delay(.4)
    restored=ai.get_camp_stock_belief("wood")
    check("belief_provenance_restored",restored.value==actual1 and "FIRSTHAND" in str(restored.source).upper())
    check("world_and_belief_restore_together",store.get_item_count("wood")==actual2)

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
        if time.monotonic()-started>120: raise TimeoutError("TASK-033 belief PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline: raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
