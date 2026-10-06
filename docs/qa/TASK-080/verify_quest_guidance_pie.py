"""Real natural-map PIE and normal new game. Teleports are explicit inspection fixtures; relic pickup uses actual campaign interaction."""
from pathlib import Path
import json,time,traceback
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
visual='-HearthwardMarkerVisual' in unreal.SystemLibrary.get_command_line()
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-080'/('visual' if visual else 'pie')
out.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
report={'ok':False,'checks':{},'method':__doc__,'markers':[]}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>=end
def shot(world,pc,name):
    unreal.SystemLibrary.execute_console_command(world,f'Shot SHOWUI filename="{(out/name).as_posix()}.png" -nosuffix',pc)
def run():
    editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.HearthwardGameMode)
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(4)
    world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0)
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    ui=pc.get_hud().get_editor_property('screen')
    game=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    campaign=next(x for x in unreal.ObjectIterator(unreal.HearthwardCampaignSubsystem) if x.get_outer()==world)
    check('normal new game',ui.execute_action('new'))
    yield lambda:not campaign.busy();yield delay(4)
    marker=json.loads(ui.describe_quest_guidance());report['markers'].append(marker)
    check('opening points at relic',marker['visible'] and marker['location']=='prologue_relic')
    target=unreal.Vector(*marker['world'])
    check('actual distance',abs(marker['distance_m']-(target-pawn.get_actor_location()).length()/100)<.1)
    # Inspection camera positions only; no level or save asset is written.
    pawn.set_actor_location(target+unreal.Vector(0,-650,100),False,True)
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=90));yield delay(1)
    shot(world,pc,'relic-front');yield delay(.5)
    check('capture 4:3 widget',ui.capture_ui('TASK-080-4x3',1024,768))
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=-90));yield delay(1)
    check('capture behind marker',ui.capture_ui('TASK-080-behind',1672,941))
    shot(world,pc,'relic-behind');yield delay(.5)
    check('turning keeps same target',json.loads(ui.describe_quest_guidance())['location']=='prologue_relic')
    if visual:
        report['ok']=True
        return
    pawn.set_actor_location(target+unreal.Vector(0,-60,100),False,True);yield delay(.3)
    check('actual relic interaction',campaign.interact());yield delay(.4)
    marker=json.loads(ui.describe_quest_guidance());report['markers'].append(marker)
    check('stage follows exit',marker['visible'] and marker['location']=='prologue_exit')
    pc.set_control_rotation(unreal.Rotator(pitch=0,yaw=90));yield delay(1)
    check('capture exit marker',ui.capture_ui('TASK-080-exit',1672,941))
    shot(world,pc,'exit-direction');yield delay(.5)
    check('cancel tracking',game.track('main_01'))
    check('untracked hides marker',not json.loads(ui.describe_quest_guidance())['visible'])
    check('resume tracking',game.track('main_01'))
    check('tracking restores marker',json.loads(ui.describe_quest_guidance())['visible'])
    ui.open_page('journal')
    check('journal hides world marker',not json.loads(ui.describe_quest_guidance())['visible'])
    ui.open_page('hud')
    check('HUD restores marker',json.loads(ui.describe_quest_guidance())['visible'])
    # Explicit reward-state fixture to protect against a stale tracked completed quest.
    game.set_editor_property('claimed',list(game.get_editor_property('claimed'))+['main_01'])
    check('claimed task hides stale target',not json.loads(ui.describe_quest_guidance())['visible'])
    report['ok']=True
runner=run();pending=None;deadline=time.monotonic()+240
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Quest guidance PIE')
        if pending and not pending():return
        pending=next(runner);return
    except StopIteration:pass
    except Exception:report['error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
