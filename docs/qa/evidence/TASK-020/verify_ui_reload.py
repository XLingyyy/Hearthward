"""Restore TASK-020 gameplay from disk in a fresh editor process."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
expected=json.loads((out/'resume.json').read_text(encoding='utf-8'))
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'passed':False,'checks':{}}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def run():
    levels.editor_request_begin_play()
    yield lambda:levels.is_in_play_in_editor()
    end=time.monotonic()+3;yield lambda:time.monotonic()>end
    w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    p=unreal.GameplayStatics.get_player_pawn(w,0)
    g=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    ui=unreal.GameplayStatics.get_player_controller(w,0).get_hud().screen
    check('continue_before_new_campaign',ui.execute_action('continue'))
    save=next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer()==w)
    saved=next(point for point in save.get_points() if point.save_id.to_string().lower()==expected['point'].lower())
    check('load_prior_process_point',ui.execute_action('load:'+saved.save_id.to_string()))
    check('xp_persisted',g.experience==expected['experience'])
    check('opponents_persisted',{str(k):v for k,v in g.opponents.items()}==expected['opponents'])
    check('durability_persisted',{str(k):v for k,v in g.durability.items()}==expected['durability'])
    check('equipment_persisted',str(g.equipment.get('weapon'))=='axe')
    check('activated_points_persisted','watch' in [str(x) for x in g.activated])
    check('new_after_continue_uses_initial_world',ui.execute_action('new') and g.enabled and g.experience==0 and not g.equipment)
    ui.open_page('dialogue')
    report['passed']=True
    (out/'reload-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
iterator=run();pending=None;deadline=time.monotonic()+90
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('reload verification')
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        (out/'reload-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
