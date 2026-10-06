"""Natural map normal new game; UI-only inspection. Camera rotation is a visual fixture. No level assets or user saves written."""
from pathlib import Path
import json,time,traceback
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_dir())/'.agent-local/qa/TASK-081/visual';out.mkdir(parents=True,exist_ok=True)
report={'ok':False,'checks':{},'method':__doc__}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
def check(k,v):
    report['checks'][k]=bool(v)
    if not v:raise AssertionError(k)
def delay(seconds):
    until=time.monotonic()+seconds
    return lambda:time.monotonic()>=until
def run():
    editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.HearthwardGameMode)
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(3)
    world=editor.get_game_world();pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().get_editor_property('screen')
    campaign=next(x for x in unreal.ObjectIterator(unreal.HearthwardCampaignSubsystem) if x.get_outer()==world)
    check('normal new game',ui.execute_action('new'));yield lambda:not campaign.busy();yield delay(3)
    pc.set_control_rotation(unreal.Rotator(pitch=-8,yaw=35));ui.open_page('dialogue');yield delay(.5)
    check('world still running during dialogue',not unreal.GameplayStatics.is_game_paused(world))
    check('natural world dialogue capture',ui.capture_ui('TASK-081-natural-home',1672,941))
    unreal.SystemLibrary.execute_console_command(world,f'Shot SHOWUI filename="{(out/"dialogue-world.png").as_posix()}" -nosuffix',pc);yield delay(1)
    ui.execute_action('dialogue.gather');check('natural gather form',ui.capture_ui('TASK-081-natural-gather',1672,941))
    ui.execute_action('dialogue.home');ui.execute_action('dialogue.team');check('natural team form',ui.capture_ui('TASK-081-natural-team',1672,941))
    check('stone selection',ui.execute_action('dialogue.item'))
    check('stone text visible','石材' in ui.describe_layout())
    check('4:3 team form',ui.capture_ui('TASK-081-natural-team-4x3',1024,768))
    ui.execute_action('dialogue.home');ui.execute_action('dialogue.more');ui.execute_action('agentInventory')
    ai=next(x for x in unreal.ObjectIterator(unreal.HearthwardLocalAISubsystem) if x.get_outer()==world)
    reply=ai.get_npc_line();ui.execute_action('dialogue.reply');ui.execute_action('dialogue.home')
    check('returning home retains reply',reply==ai.get_npc_line() and bool(reply))
    check('back to gameplay',ui.execute_action('back') and str(ui.get_page())=='hud')
    report['ok']=True
runner=run();pending=None;deadline=time.monotonic()+300
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('Dialogue081 visual')
        if pending and not pending():return
        pending=next(runner);return
    except StopIteration:pass
    except Exception:report['error']=traceback.format_exc()
    (out/'results.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play()
handle=unreal.register_slate_post_tick_callback(tick)
