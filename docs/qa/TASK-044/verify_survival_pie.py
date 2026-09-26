"""TASK-044 runtime fixture: real PIE actors/input/actions/save; no level assets modified."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/"Saved/Task044/pie"
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={"ok":False,"checks":{},"method":"PIE native APIs plus Enhanced Input movement/camera injection; no physical keyboard claim"}
st={"phase":"start","deadline":time.monotonic()+180}
def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def phase(name): st.update(phase=name,since=time.monotonic())
def state(): return st["survival"].get_editor_property("state")
def active(): return st["clock"].get_snapshot().active_play_seconds
def shot(name):
    unreal.SystemLibrary.execute_console_command(st["world"],f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',st["pc"])
def inject(action,x=0,y=0): st["input"].inject_input_vector_for_action(action,unreal.Vector(x,y,0),[],[])
def finish(error=None):
    if error: report["error"]=error
    report["ok"]=not error and all(report["checks"].values())
    (out/"results.json").write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        unreal.GameplayStatics.set_game_paused(st["world"],False)
        levels.editor_request_end_play()
def tick(dt):
    try:
        now=time.monotonic()
        if now>st["deadline"]: raise TimeoutError(st["phase"])
        p=st["phase"]
        if p=="start": levels.editor_request_begin_play();phase("possess")
        elif p=="possess":
            if not levels.is_in_play_in_editor(): return
            w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn=unreal.GameplayStatics.get_player_pawn(w,0)
            if not pawn:return
            pc=unreal.GameplayStatics.get_player_controller(w,0)
            g=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
            sv=pawn.get_component_by_class(unreal.HearthwardSurvivalComponent)
            save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer()==w)
            clock=next(x for x in unreal.ObjectIterator(unreal.HearthwardWorldClockSubsystem) if x.get_outer()==w)
            screen=pc.get_hud().get_editor_property("screen")
            unreal.GameplayStatics.set_game_paused(w,False)
            unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.CreateTest",pc)
            brother=unreal.GameplayStatics.get_actor_of_class(w,unreal.HearthwardCompanionFixture)
            check("brother fixture exists",brother is not None)
            brother.set_actor_location(pawn.get_actor_location()+unreal.Vector(150,0,0),False,True)
            actions=sorted((a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==pawn and a.get_name().startswith("InputAction")),key=lambda a:a.get_name())
            inp=next(x for x in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if isinstance(x.get_outer(),unreal.LocalPlayer))
            st.update(world=w,pawn=pawn,pc=pc,g=g,survival=sv,save=save,clock=clock,screen=screen,brother=brother,
                      bsv=brother.get_component_by_class(unreal.HearthwardSurvivalComponent),bag=pawn.get_component_by_class(unreal.HearthwardInventoryComponent),input=inp,move=actions[0],look=actions[1])
            phase("ground")
        elif p=="ground":
            if now-st["since"]<1:return
            st["g"].set_editor_property("enabled",True)
            captured=st["save"].enable_prototype()
            report["capture_status"]=st["save"].get_status()
            report["positions"]={"player":str(st["pawn"].get_actor_location()),"brother":str(st["brother"].get_actor_location()),"player_mode":str(st["pawn"].character_movement.movement_mode),"brother_mode":str(st["brother"].character_movement.movement_mode)}
            check("save participant capture",captured)
            check("initial progress",st["save"].start_new_progress())
            st["screen"].open_page("hud")
            st["g"].set_editor_property("health",40)
            st["bag"].try_add("medicine",2)
            check("medicine starts",st["g"].use_item("medicine"))
            st["started"]=active();phase("medicine")
        elif p=="medicine":
            inject(st["look"],.1,0)
            if active()-st["started"]<.7:return
            check("camera preserves action",str(state().get_editor_property("medicine"))=="medicine")
            st["screen"].open_page("pause")
            st["frozen"]=(active(),state().get_editor_property("medicine_remaining"))
            phase("paused")
        elif p=="paused":
            if now-st["since"]<1:return
            check("pause freezes both clock and medicine",st["frozen"]==(active(),state().get_editor_property("medicine_remaining")))
            st["screen"].open_page("hud")
            st["g"].apply_damage(5)
            check("hit leaves half dose",st["bag"].get_item_count("medicine")==1 and st["bag"].get_item_count("medicine_half")==1)
            shot("half-dose")
            check("half dose starts",st["g"].use_item("medicine_half"))
            st["hp"]=st["g"].health;st["started"]=active();phase("half")
        elif p=="half":
            if active()-st["started"]<3.15:return
            check("half dose completed",st["bag"].get_item_count("medicine_half")==0 and st["g"].health>=st["hp"]+15)
            check("next full dose starts",st["g"].use_item("medicine"));st["move_frames"]=0;phase("move")
        elif p=="move":
            inject(st["move"],0,1)
            st["move_frames"]+=1
            if st["move_frames"]<3:return
            inject(st["move"])
            check("movement cancels without spending",str(state().get_editor_property("medicine"))=="None" and st["bag"].get_item_count("medicine")==1)
            phase("settle")
        elif p=="settle":
            if now-st["since"]<.7:return
            st["brother"].set_actor_location(st["pawn"].get_actor_location()+unreal.Vector(150,0,0),False,True)
            st["g"].apply_damage(10000)
            check("player downed",state().life==unreal.HearthwardLife.DOWNED)
            st["started"]=active();st["down_shot"]=False;phase("rescue")
        elif p=="rescue":
            if not st["down_shot"]:
                shot("player-downed");st["down_shot"]=True
            if active()-st["started"]<6:return
            check("brother really rescues",state().life==unreal.HearthwardLife.ALIVE and st["g"].health>=10)
            st["brother"].set_actor_location(st["pawn"].get_actor_location()+unreal.Vector(150,0,0),False,True)
            unreal.GameplayStatics.apply_damage(st["brother"],10000,st["pc"],None,unreal.DamageType)
            check("brother downed",st["bsv"].state.life==unreal.HearthwardLife.DOWNED)
            report["rescue_context"]={"player":str(st["pawn"].get_actor_location()),"brother":str(st["brother"].get_actor_location()),"mode":str(st["pawn"].character_movement.movement_mode),"state":str(state())}
            check("player starts rescue",st["survival"].begin_rescue(st["bsv"]))
            st["started"]=active();phase("rescue_brother")
        elif p=="rescue_brother":
            if active()-st["started"]<5.2:return
            check("player really rescues",st["bsv"].state.life==unreal.HearthwardLife.ALIVE)
            st["g"].set_editor_property("health",40)
            check("medicine before save",st["g"].use_item("medicine"))
            check("save pending medicine",st["save"].save_point(True))
            st["point"]=st["save"].get_points()[-1].save_id;st["started"]=active();phase("saved_medicine")
        elif p=="saved_medicine":
            if active()-st["started"]<3.2:return
            check("load pending action",st["save"].load_point(st["point"]))
            check("restore reservation and action",str(state().get_editor_property("medicine"))=="medicine" and st["bag"].get_item_count("medicine")==1)
            st["g"].apply_damage(10000)
            unreal.GameplayStatics.apply_damage(st["brother"],10000,st["pc"],None,unreal.DamageType)
            phase("failed")
        elif p=="failed":
            if now-st["since"]<.5:return
            check("both downed opens paused failure load",str(st["screen"].get_page())=="save" and unreal.GameplayStatics.is_game_paused(st["world"]))
            shot("failure-load");phase("failure_capture")
        elif p=="failure_capture":
            if now-st["since"]<1:return
            check("failure can load valid node",st["save"].load_point(st["point"]))
            st["screen"].open_page("hud")
            check("load restores alive brothers",state().life==unreal.HearthwardLife.ALIVE and st["bsv"].state.life==unreal.HearthwardLife.ALIVE)
            finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
