"""TASK-029 legacy dialogue presentation verifies the same explicit suggestion cache."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
out=Path(unreal.Paths.project_dir())/"Saved/Task029";out.mkdir(parents=True,exist_ok=True)
result=out/"legacy-suggestions-pie-results.json"
report={"ok":False,"checks":{}}

def check(name,value):
    report["checks"][name]=bool(value)
    if not value: raise AssertionError(name)
def wait(pred,seconds=30):return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end,seconds+5)
def sub(cls,w):return next(x for x in unreal.ObjectIterator(cls) if x.get_outer()==w)

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(1)
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(w,0);p=unreal.GameplayStatics.get_player_pawn(w,0);hud=pc.get_hud()
    check("legacy_screen_disabled",hud.screen is None)
    unreal.SystemLibrary.execute_console_command(w,"Hearthward.Companion.CreateTest",pc);yield delay(.5)
    companions=unreal.GameplayStatics.get_all_actors_with_tag(w,"Hearthward.Companion.PROTOTYPE_ONLY")
    check("fixture_created",len(companions)==1)
    c=companions[0];p.set_actor_location(c.get_actor_location()+unreal.Vector(-150,0,0),False,True)
    hud.toggle_dialogue();yield delay(.3)
    widget=hud.get_dialogue_widget()
    check("legacy_dialogue_open",widget is not None)
    initial=widget.get_displayed_suggestions()
    check("legacy_starts_unrefreshed",len(initial)==3 and all("尚未刷新" in x for x in initial))
    widget.refresh_suggestion_choices();yield delay(.3)
    ai=sub(unreal.HearthwardLocalAISubsystem,w);suggestions=ai.get_suggestions()
    check("legacy_explicit_refresh",len(suggestions)==3)
    displayed=[str(x) for x in widget.get_displayed_suggestions()]
    expected=[str(x.label) for x in suggestions]
    report["displayed"]=displayed;report["expected"]=expected
    check("legacy_shows_three",len(displayed)==3 and len(suggestions)==3)
    check("legacy_labels_match_shared_cache",displayed==expected)
    check("legacy_refresh_no_candidate",not ai.has_candidate())
    hud.close_dialogue()
    levels.editor_request_end_play();yield wait(lambda:not levels.is_in_play_in_editor(),20)

def finish(error=None):
    if error:report["error"]=error
    report["ok"]=not error and bool(report["checks"]) and all(report["checks"].values())
    result.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
flow=run();pending=None;started=time.monotonic()
def tick(_dt):
    global pending
    try:
        if time.monotonic()-started>90:raise TimeoutError("legacy suggestion PIE timeout")
        if pending:
            pred,deadline=pending
            if not pred():
                if time.monotonic()>deadline:raise TimeoutError("wait expired")
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:finish(traceback.format_exc())
handle=unreal.register_slate_post_tick_callback(tick)
