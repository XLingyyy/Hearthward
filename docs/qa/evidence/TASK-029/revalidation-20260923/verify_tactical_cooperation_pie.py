"""TASK-036 PIE: assist dynamically becomes protect/regroup using existing UE combat authority."""
import json,time,traceback,math
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task036";out.mkdir(parents=True,exist_ok=True)
result_path=out/"tactical-cooperation-pie-results.json"
report={"ok":False,"checks":{},"samples":[]}
st={}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=30): return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def dist2d(a,b):
    return math.hypot(a.x-b.x,a.y-b.y)

def sample(g,c,label):
    report["samples"].append({
        "label":label,
        "health":float(g.health),
        "health_ratio":float(g.health/g.max_health()) if g.max_health()>0 else 0,
        "intent":str(g.get_companion_tactical_intent()),
        "target":str(g.get_companion_combat_target()),
        "reason":g.get_companion_combat_reason(),
        "companion_block":c.block_reason,
        "companion_location":str(c.get_actor_location())
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
    g=p.get_component_by_class(unreal.HearthwardGameplayComponent);st["g"]=g;st["c"]=c
    camp=c.camp.get_actor_location()

    # Known encounter geometry: guard_1 is camp+(1000,-600), guard_2 camp+(1400,-500).
    guard1=camp+unreal.Vector(1000,-600,0)
    healthy_player=guard1+unreal.Vector(-280,0,0)
    p.set_actor_location(healthy_player,False,True)
    # Keep the companion inside the deterministic attack stop distance. This verifies
    # real attack settlement without making the check depend on navmesh warm-up.
    c.set_actor_location(guard1+unreal.Vector(-150,0,0),False,True)
    yield delay(.2)
    check("assist_directive",g.apply_companion_directive(p,"assist"))
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="assist",4)
    sample(g,c,"healthy_assist")
    check("healthy_assist_target",str(g.get_companion_combat_target()).lower()=="guard_1")
    before=float(g.opponents.get("guard_1",0))
    yield wait(lambda:float(g.opponents.get("guard_1",0))<before,8)
    after=float(g.opponents.get("guard_1",0))
    check("assist_uses_real_damage",after<before)

    # Lower player health at a safe distance, then move back near exactly one threat.
    p.set_actor_location(camp+unreal.Vector(-500,0,0),False,True)
    c.set_actor_location(p.get_actor_location()+unreal.Vector(-120,0,0),False,True)
    for _ in range(12):
        if g.health/g.max_health()<=.4: break
        g.apply_damage(8)
    check("player_is_low_health",0<g.health/g.max_health()<=.5)
    p.set_actor_location(healthy_player,False,True)
    c.set_actor_location(guard1+unreal.Vector(-150,0,0),False,True)
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="protect",4)
    sample(g,c,"low_health_protect")
    check("protect_targets_close_threat",str(g.get_companion_combat_target()).lower()=="guard_1")
    check("protect_reason",g.get_companion_combat_reason()=="PROTECT_LOW_HEALTH")
    protect_before=float(g.opponents.get("guard_1",0))
    yield wait(lambda:float(g.opponents.get("guard_1",0))<protect_before,8)
    protect_after=float(g.opponents.get("guard_1",0))
    check("protect_still_uses_real_attack_settlement",protect_after<protect_before)

    # Position both guard_1 and guard_2 within 300cm but outside their 240cm player attack radius.
    regroup_player=camp+unreal.Vector(1154,-367,0)
    p.set_actor_location(regroup_player,False,True)
    c.set_actor_location(regroup_player+unreal.Vector(500,0,0),False,True)
    start_dist=dist2d(c.get_actor_location(),p.get_actor_location())
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="regroup",4)
    sample(g,c,"overwhelmed_regroup_start")
    check("regroup_has_no_target",str(g.get_companion_combat_target()).lower() in ("none",""))
    check("regroup_reason",g.get_companion_combat_reason()=="LOW_HEALTH_OVERWHELMED")
    yield delay(1.3)
    end_dist=dist2d(c.get_actor_location(),p.get_actor_location())
    report["regroup_distance"]={"start":start_dist,"end":end_dist}
    sample(g,c,"overwhelmed_regroup_after_move")
    check("regroup_moves_toward_player",end_dist+30<start_dist)
    check("player_survives_regroup_probe",g.health>0)

    # Downed player: assist no longer keeps chasing. Companion collapses back toward the player.
    p.set_actor_location(camp+unreal.Vector(-700,150,0),False,True)
    c.set_actor_location(p.get_actor_location()+unreal.Vector(500,0,0),False,True)
    while g.health>0:
        g.apply_damage(1000)
    check("player_downed",g.health<=0)
    down_start=dist2d(c.get_actor_location(),p.get_actor_location())
    yield wait(lambda:str(g.get_companion_tactical_intent()).lower()=="regroup",4)
    sample(g,c,"player_down_regroup_start")
    check("down_regroup_reason",g.get_companion_combat_reason()=="PLAYER_DOWN_REGROUP")
    yield delay(1.1)
    down_end=dist2d(c.get_actor_location(),p.get_actor_location())
    report["down_regroup_distance"]={"start":down_start,"end":down_end}
    check("downed_regroup_moves_toward_player",down_end+25<down_start)
    check("downed_regroup_has_no_target",str(g.get_companion_combat_target()).lower() in ("none",""))

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
        if time.monotonic()-started>120: raise TimeoutError("TASK-036 tactical cooperation PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:
                    g=st.get("g")
                    raise TimeoutError("wait expired intent="+(str(g.get_companion_tactical_intent()) if g else "startup"))
                return
        pending=next(flow)
    except StopIteration: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
