"""Rendered production map; normal campaign entry, movement input and real facilities.
Fixture inventory grants prepare construction; independent GUID save pool only.
Physical keyboard checks are recorded separately from these API operations.
"""
import json,time,traceback,re
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir())
out=root/"Saved/Task052/pie";out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={"ok":False,"checks":{},"method":__doc__,"stage":"starting"}
st={}
def check(name,value):
    report["checks"][name]=bool(value)
    if not value:raise AssertionError(name)
def publish():
    (out/"progress.json").write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
def wait(predicate,seconds=90):
    publish()
    return predicate,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+10)
def sub(cls):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==st["world"])
def state(key):
    return json.loads(st[key].describe())
def clock():
    return st["clock"].get_snapshot()
def guid(text):
    v=unreal.GuidLibrary.parse_string_to_guid(text);return v[0] if isinstance(v,tuple) else v
def guid_text(value):
    return "".join(f"{value.get_editor_property(k)&0xffffffff:08X}" for k in ("a","b","c","d"))
def shot(name):
    unreal.SystemLibrary.execute_console_command(st["world"],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st["pc"])
    (root/"Saved/Task020").mkdir(parents=True,exist_ok=True)
    check("UI capture "+name,st["ui"].capture_ui("TASK052-"+name,1920,1080))
def move(x,y):
    p=st["pawn"].get_actor_location();d=unreal.Vector(x-p.x,y-p.y,0)
    if d.length()<12:return True
    st["pc"].set_control_rotation(unreal.Rotator(pitch=-8,yaw=unreal.MathLibrary.deg_atan2(d.y,d.x)))
    st["pawn"].add_movement_input(d/d.length(),1,False);return False
def request(kind,facility,minutes,operation=None):
    return unreal.HearthwardTimeAdvanceRequest(kind=kind,facility=guid(facility),campaign=st["save"].get_campaign_id(),epoch=st["store"].get_timeline_epoch(),operation_id=operation or unreal.GuidLibrary.new_guid(),start_w=clock().elapsed_calendar_minutes,minutes=minutes)
def point():
    old={guid_text(x.save_id) for x in st["save"].get_points()}
    check("safe checkpoint write",st["save"].save_point(True))
    return next(x.save_id for x in st["save"].get_points() if guid_text(x.save_id) not in old)
def finish(error=None):
    if error:report["error"]=error
    report["ok"]=not error and all(report["checks"].values())
    try:
        report["clock"]={"a":clock().active_play_seconds,"w":clock().elapsed_calendar_minutes,"day":clock().display_day,"minute":clock().minute_of_day}
        report["save_status"]=st["save"].get_status()
        report["feedback"]=str(st["camp"].feedback)
        report["facilities"]=state("camp")["facilities"]
        report["player_position"]=str(st["pawn"].get_actor_location())
        report["building_positions"]=[str(x.get_actor_location()) for x in st["builder"].get_buildings()]
    except Exception:pass
    (out/"results.json").write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    publish()
    unreal.unregister_slate_post_tick_callback(handle)
    # Leave a paused game window for independent physical-input inspection.
    if st.get("ui"):st["ui"].open_page("save")
def run():
    editor.get_editor_world().get_world_settings().set_editor_property("default_game_mode",unreal.load_class(None,"/Script/Hearthward.HearthwardGameMode"))
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(8)
    w=editor.get_game_world();p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
    st.update(world=w,pawn=p,pc=pc,ui=pc.get_hud().get_editor_property("screen"),game=p.get_component_by_class(unreal.HearthwardGameplayComponent),bag=p.get_component_by_class(unreal.HearthwardInventoryComponent),builder=p.get_component_by_class(unreal.HearthwardBuildingComponent))
    for key,cls in [("clock",unreal.HearthwardWorldClockSubsystem),("campaign",unreal.HearthwardCampaignSubsystem),("camp",unreal.HearthwardCampSubsystem),("store",unreal.HearthwardStorageSubsystem),("save",unreal.HearthwardSaveSubsystem)]:st[key]=sub(cls)
    st["loading"]=next(x for x in unreal.ObjectIterator(unreal.HearthwardLoadingSubsystem) if x.get_outer()==unreal.GameplayStatics.get_game_instance(w))
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Clock.CheckMenuKeyboard",pc)
    report["keyboard_method"]="synthetic engine key events; desktop input unavailable"
    check("menu keyboard new-game and confirmation",json.loads((root/"Saved/Task052/keyboard.json").read_text(encoding="utf-8-sig"))["passed"])
    has_campaign=any(st["save"].get_campaign_id().get_editor_property(k)!=0 for k in ("a","b","c","d"))
    check("normal new-game entry",has_campaign or st["ui"].execute_action("new"))
    start=clock()
    def loaded():
        now=clock()
        if st["loading"].is_loading():
            check("loading freezes action clock",now.active_play_seconds==start.active_play_seconds)
            check("loading freezes calendar",now.elapsed_calendar_minutes==start.elapsed_calendar_minutes)
            return False
        return True
    yield wait(loaded,180);yield delay(2)
    check("prologue on loaded terrain",state("campaign")["phase"]=="prologue" and not p.get_movement_component().is_falling())
    check("new progress starts at 20:00",clock().minute_of_day>=1200 and clock().minute_of_day<1220)
    shot("prologue-night")
    check("relic interaction",st["campaign"].interact())
    check("brother explicit follow",st["game"].order_companion("follow"))
    for x,y in [(92000,53364),(92400,53364),(94000,53500),(94000,57000),(94000,62000),(90000,62000)]:
        yield wait(lambda x=x,y=y:move(x,y),100)
    st["brother"]=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    yield wait(lambda:(st["brother"].get_actor_location()-p.get_actor_location()).length()<900,60)
    check("exit interaction",st["campaign"].interact())
    yield wait(lambda:state("campaign")["phase"]=="occupied" and not st["campaign"].busy(),180)
    yield delay(3)
    st["game"].order_companion("hold")
    report["stage"]="constructing real facilities";publish()
    st["bag"].try_add("wood",16);st["bag"].try_add("rope",2);st["bag"].try_add("stone",4)
    for kind in ["bed","campfire"]:
        check("select "+kind,st["builder"].select_building(kind))
        valid=False
        for yaw in [0,90,180,270,45,135,225,315]:
            pc.set_control_rotation(unreal.Rotator(pitch=-25,yaw=yaw));yield delay(.4)
            if st["builder"].get_editor_property("valid_placement"):valid=True;break
        report[kind+"_placement_feedback"]=str(st["builder"].feedback)
        check(kind+" natural placement",valid)
        check(kind+" construction starts",st["builder"].confirm_placement())
        yield wait(lambda:not st["builder"].is_building(),15)
        check(kind+" facility registered",any(x["kind"]==kind for x in state("camp")["facilities"]))
        # Move away from the first structure before placing the next one.
        if kind=="bed":
            here=p.get_actor_location()
            yield wait(lambda:move(here.x-220,here.y),5)
    yield delay(.8)
    report["stage"]="time transactions";publish()
    facilities=state("camp")["facilities"]
    bed=next(x for x in facilities if x["kind"]=="bed")
    fire=next(x for x in facilities if x["kind"]=="campfire")
    actors=st["builder"].get_buildings()
    for kind,fac in [("bed",bed),("campfire",fire)]:
        a=actors[facilities.index(fac)];at=a.get_actor_location()
        here=p.get_actor_location()
        yield wait(lambda:move(here.x,at.y-420),10)
        yield wait(lambda:move(at.x,at.y-280),10)
        yield delay(.5)
        if kind=="bed":
            R=request(unreal.HearthwardTimeAdvanceKind.SLEEP,fac["id"],480);before=clock()
            reply=st["clock"].request_time_advance(R)
            report["sleep_receipt"]={"accepted":reply.accepted,"completed":reply.completed,"advanced":reply.committed_minutes,"reason":reply.reason}
            check("real bed advances eight hours",reply.accepted and reply.completed and reply.committed_minutes==480)
            check("sleep does not consume A",clock().active_play_seconds==before.active_play_seconds)
            after=clock();again=st["clock"].request_time_advance(R)
            check("duplicate sleep returns receipt without time",again.committed_minutes==480 and clock().elapsed_calendar_minutes==after.elapsed_calendar_minutes)
            R.minutes=60
            check("changed duplicate is rejected",not st["clock"].request_time_advance(R).accepted)
            shot("after-sleep-night")
            yield delay(.5)
            before_two=clock()
            for i in range(2):
                extra=st["clock"].request_time_advance(request(unreal.HearthwardTimeAdvanceKind.SLEEP,fac["id"],480))
                check("consecutive sleep "+str(i+2),extra.accepted and extra.completed and extra.committed_minutes==480)
            check("three sleeps add 1440 without A",clock().elapsed_calendar_minutes==before_two.elapsed_calendar_minutes+960 and clock().active_play_seconds==before_two.active_play_seconds)
            st["checkpoint"]=point()
            st["snapshot_w"]=clock().elapsed_calendar_minutes
            st["old_request"]=R
        else:
            for minutes in [60,240,480]:
                before=clock()
                R=request(unreal.HearthwardTimeAdvanceKind.CAMPFIRE,fac["id"],minutes)
                result=st["clock"].request_time_advance(R)
                check("campfire "+str(minutes),result.accepted and result.committed_minutes==minutes and clock().active_play_seconds==before.active_play_seconds)
            before=clock()
            bad=st["clock"].request_time_advance(request(unreal.HearthwardTimeAdvanceKind.CAMPFIRE,fac["id"],61))
            check("invalid wait is rejected without time",not bad.accepted and str(bad.reason_code)=="INVALID_DURATION" and clock().elapsed_calendar_minutes==before.elapsed_calendar_minutes)
            shot("camp-daylight")
    st["ui"].open_page("camp");st["ui"].execute_action("camp.tab:facilities");st["ui"].execute_action("camp.facility:"+fire["id"])
    shot("campfire-menu");yield delay(.5)
    frozen=clock();yield delay(1)
    check("menu freezes both clocks",clock().active_play_seconds==frozen.active_play_seconds and clock().elapsed_calendar_minutes==frozen.elapsed_calendar_minutes)
    paused=st["clock"].request_time_advance(request(unreal.HearthwardTimeAdvanceKind.CAMPFIRE,fire["id"],60))
    check("paused time request rejected",not paused.accepted and str(paused.reason_code)=="PAUSED" and clock().elapsed_calendar_minutes==frozen.elapsed_calendar_minutes)
    st["ui"].open_page("hud")
    check("restore real checkpoint",st["save"].load_point(st["checkpoint"]))
    check("W restores without future minutes",clock().elapsed_calendar_minutes==st["snapshot_w"])
    check("old epoch request rejected after restore",not st["clock"].request_time_advance(st["old_request"]).accepted)
    check("repeat checkpoint restore",st["save"].load_point(st["checkpoint"]))
    check("repeated restore does not advance W",clock().elapsed_calendar_minutes==st["snapshot_w"])
    shot("restored-night");yield delay(.5)
    # Explicit battle/invalid-ground fixtures in a GUID pool; production station travel runs unchanged.
    for mode in ["down","stay","blocked"]:
        report["stage"]="travel fixture "+mode;publish()
        check("restore before travel "+mode,st["ui"].execute_action("load:"+guid_text(st["checkpoint"])))
        st["ui"].open_page("hud")
        yield wait(lambda:not st["loading"].is_loading(),180);yield delay(.5)
        station=state("campaign")["positions"]["camp"]
        yield wait(lambda:move(station["x"],station["y"]-165),10)
        check("player at genuine origin station "+mode,str(st["game"].nearby_location())=="camp")
        st["game"].order_companion("hold")
        unreal.SystemLibrary.execute_console_command(w,"Hearthward.Clock.TravelFixture "+mode,pc)
        prior=json.loads((root/"Saved/Task052/travel-before.json").read_text(encoding="utf-8-sig"))
        old_p=p.get_actor_location();old_b=st["brother"].get_actor_location();before=clock()
        check("real station travel starts "+mode,st["game"].travel("route_mine"))
        def arrived():
            now=clock()
            check("travel A frozen "+mode,now.active_play_seconds==before.active_play_seconds)
            check("travel W frozen "+mode,now.elapsed_calendar_minutes==before.elapsed_calendar_minutes)
            return not st["campaign"].busy()
        yield wait(arrived,180)
        after=state("campaign")
        fields=[e for e in prior["enemies"] if e["group"]=="field"][:3]
        current={e["id"]:e for e in after["enemies"]}
        a,b,c=[current[e["id"]] for e in fields]
        check("unengaged enemy keeps HP "+mode,b["combat"]["health"]==fields[1]["combat"]["health"])
        check("stunned enemy stays cleared "+mode,c["combat"]["health"]==0 and c["combat"]["bStunned"])
        for e in fields:
            t=current[e["id"]]
            check("enemy state preserved "+mode+" "+e["id"],all(t["combat"][k]==e["combat"][k] for k in ["generation","armorDurability","hitRemaining","investigationRemaining","broadcastRegions"]))
        if mode=="blocked":
            check("failed travel keeps player",(p.get_actor_location()-old_p).length()<.01)
            check("failed travel keeps brother",(st["brother"].get_actor_location()-old_b).length()<.01)
            check("failed travel keeps enemy HP",a["combat"]["health"]==fields[0]["combat"]["health"])
        else:
            check("player reached new station "+mode,(p.get_actor_location()-old_p).length()>10000)
            check("participating enemy heals "+mode,a["combat"]["health"]>fields[0]["combat"]["health"])
            if mode=="down":
                survival=st["brother"].get_component_by_class(unreal.HearthwardSurvivalComponent).get_editor_property("state")
                check("downed brother follows without countdown",survival.life==unreal.HearthwardLife.DOWNED and float(re.search(r"DownRemaining=([0-9.]+)",survival.export_text()).group(1))==45 and (st["brother"].get_actor_location()-p.get_actor_location()).length()<1000)
                check("downed arrival cannot save",not st["save"].save_point(True))
            else:check("independent brother stays",(st["brother"].get_actor_location()-old_b).length()<.01)
        (out/("travel-"+mode+".json")).write_text(json.dumps({"fixture":mode,"before":prior,"after":after},ensure_ascii=False,indent=2),encoding="utf-8")
        yield wait(lambda:not st["loading"].is_loading(),180)
    check("final checkpoint restore",st["ui"].execute_action("load:"+guid_text(st["checkpoint"])))
    st["ui"].open_page("hud");yield wait(lambda:not st["loading"].is_loading(),180)
    report["stage"]="completed";finish()
runner=run();pending=None
def tick(delta):
    global pending
    try:
        if pending:
            predicate,deadline=pending
            if not predicate():
                if time.monotonic()>deadline:raise TimeoutError("stage "+report["stage"])
                return
        pending=next(runner)
    except StopIteration:pass
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
