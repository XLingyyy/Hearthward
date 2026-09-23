"""Natural-map production New Game -> real model -> execution -> save/load.

Run in an isolated HearthwardSaveTestPool; never saves editor map assets.
"""
import json
import time
import traceback
import re
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root = Path(unreal.Paths.project_dir())
out = root / 'Saved/DemoValidation/npc-smoke-diagnostic'
out.mkdir(parents=True, exist_ok=True)
workshop_only = 'HearthwardCampWorkshopTest' in unreal.SystemLibrary.get_command_line()
report = {'ok': False, 'checks': {}, 'steps': []}
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def check(name, value):
    report['checks'][name] = bool(value)
    (out / 'progress.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if not value:
        raise AssertionError(name)

def wait(pred, seconds=90):
    return pred, time.monotonic() + seconds

def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)

def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()

def sub(cls):
    return next(x for x in unreal.ObjectIterator(cls) if x.get_outer() == world())

def snapshot(label, c, ai):
    report['steps'].append(dict(label=label, position=str(c.get_actor_location()),
        phase=str(c.get_phase()), reason=c.block_reason, requested=c.get_requested(),
        delivered=c.get_delivered(), acquired=c.get_acquired(), carried=c.get_carried(),
        source=c.source.get_item_count('wood'), raw=ai.get_last_structured_result(), status=ai.get_status()))

def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor,30);yield delay(2)
    unreal.GameplayStatics.get_player_controller(world(),0).get_hud().screen.execute_action('new')
    yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_HearthwardWilds');yield delay(8)
    p=unreal.GameplayStatics.get_player_pawn(world(),0)
    c=unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.HearthwardCompanionFixture)[0]
    ai=sub(unreal.HearthwardLocalAISubsystem);store=sub(unreal.HearthwardStorageSubsystem)
    goal=unreal.HearthwardAgentGoal()
    for key,value in dict(intent='collect',item='wood',quantity=2,quantity_mode='additional_acquired',source_ref='S1').items():goal.set_editor_property(key,value)
    check('request',ai.set_structured_goal(p,c,goal));check('confirm',ai.confirm_candidate(ai.get_candidate_id()))
    
    for i in range(12):
        yield delay(5)
        snapshot('after_'+str((i+1)*5),c,ai)
        (out/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        if c.get_delivered()==2:break
    check('delivered_with_chest_collision',store.get_item_count('wood')==2 and c.source.get_item_count('wood')==14)
    snapshot('complete',c,ai)

def finish(error=None):
    if error:
        report['error'] = error
    report['ok'] = not error and bool(report['checks']) and all(report['checks'].values())
    (out / 'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    unreal.SystemLibrary.quit_editor()

flow = run()
pending = None
def tick(_dt):
    global pending
    try:
        if pending:
            pred, deadline = pending
            if not pred():
                if time.monotonic() > deadline:
                    raise TimeoutError('wait expired')
                return
        pending = next(flow)
    except StopIteration:
        finish()
    except Exception:
        finish(traceback.format_exc())

handle = unreal.register_slate_post_tick_callback(tick)
