"""Focused visual take: offset camera so the player does not occlude the fixture lane."""
import json
import time
import traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_dir()) / "Saved/Task012"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {"ok": False, "mode": "fixture", "checks": {}, "input_probe": {}}
st = {"phase": "start", "deadline": time.monotonic()+100}
P = unreal.HearthwardCompanionPhase

def phase(name): st.update(phase=name, since=time.monotonic())
def shot(name):
    unreal.SystemLibrary.execute_console_command(st["world"], f'HighResShot 1280x720 filename="{(out/(name+".png")).as_posix()}"', st["pc"])
def finish(error=None):
    if error: report["error"] = error
    report["ok"] = not error and len(report["checks"]) == 4 and all(report["checks"].values()) and all(report["input_probe"].values())
    (out/"visual-results.json").write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor(): levels.editor_request_end_play()
def tick(dt):
    try:
        if time.monotonic()>st["deadline"]: raise TimeoutError("visual take")
        p=st["phase"]
        if p=="start":
            levels.editor_request_begin_play(); phase("possess")
        elif p=="possess":
            if not levels.is_in_play_in_editor(): return
            w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            pawn=unreal.GameplayStatics.get_player_pawn(w,0)
            pc=unreal.GameplayStatics.get_player_controller(w,0)
            if not pawn or not pc: return
            st.update(world=w,pawn=pawn,pc=pc)
            phase("settle")
        elif p=="settle" and time.monotonic()-st["since"]>1:
            pawn,pc,w=st["pawn"],st["pc"],st["world"]
            pawn.set_actor_location(unreal.Vector(-450,-200,90),False,True)
            pc.set_control_rotation(unreal.Rotator(pitch=-12, yaw=25, roll=0))
            unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.CreateTest",pc)
            c=unreal.GameplayStatics.get_all_actors_with_tag(w,"Hearthward.Companion.PROTOTYPE_ONLY")[0]
            c.set_actor_location(unreal.Vector(0,400,90),False,True)
            c.camp.set_actor_location(unreal.Vector(0,400,90),False,True)
            c.source.get_owner().set_actor_location(unreal.Vector(600,400,90),False,True)
            st.update(comp=c, storage=next(x for x in unreal.ObjectIterator(unreal.HearthwardStorageSubsystem) if x.get_outer()==w))
            for raw in ["1.5", "2147483648", "999999999999999999999999", "4294967297", "-1", "0"]:
                before = (c.get_requested(), c.get_phase())
                unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.Collect "+raw,pc)
                report["input_probe"][raw] = (c.get_requested(), c.get_phase()) == before
                c.cancel(pawn)
            unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.Collect 10",pc)
            phase("outbound")
        elif p=="outbound" and st["comp"].get_actor_location().x>200:
            report["checks"]["outbound_real_position"]=st["comp"].get_phase()==P.GOING_TO_SOURCE and st["storage"].get_item_count("wood")==0
            shot("lane-outbound"); phase("returning")
        elif p=="returning" and st["comp"].get_phase()==P.RETURNING and st["comp"].get_actor_location().x<350:
            report["checks"]["return_carries_four_before_delivery"]=st["comp"].bag.get_item_count("wood")==4 and st["comp"].get_delivered()==0
            shot("lane-returning"); phase("complete")
        elif p=="complete" and st["comp"].get_phase()==P.COMPLETED:
            report["checks"]["visible_goal_completed"]=st["comp"].get_delivered()==10 and st["storage"].get_item_count("wood")==10
            shot("lane-completed"); phase("shortage_start")
        elif p=="shortage_start" and time.monotonic()-st["since"]>0.6:
            unreal.SystemLibrary.execute_console_command(st["world"],"Hearthward.Companion.Collect 8",st["pc"])
            phase("shortage")
        elif p=="shortage" and st["comp"].get_phase()==P.WAITING_AT_CAMP:
            report["checks"]["visible_shortage_waiting"]=st["comp"].get_delivered()==6 and st["storage"].get_item_count("wood")==16
            shot("lane-shortage"); phase("finish")
        elif p=="finish" and time.monotonic()-st["since"]>0.6: finish()
    except Exception: finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
