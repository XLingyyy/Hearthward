"""Natural-game UI handlers and actual viewport screenshots with Slate UI."""
import json,time,traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'SceneOverlayValidation';out.mkdir(exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report={'ok':False,'checks':{}}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def wait(pred,seconds=90):return pred,time.monotonic()+seconds
def delay(seconds):
    end=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=end)
def world():return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
def shot(name):
    unreal.SystemLibrary.execute_console_command(world(),'Shot SHOWUI filename="'+str(out/(name+'.png'))+'"')
    yield delay(.6)
def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor);yield delay(2)
    unreal.GameplayStatics.get_player_controller(world(),0).get_hud().screen.execute_action('new')
    yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_HearthwardWilds');yield delay(5)
    w=world();pc=unreal.GameplayStatics.get_player_controller(w,0);p=unreal.GameplayStatics.get_player_pawn(w,0)
    hud=pc.get_hud();ui=hud.screen;game=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    check('new_game_hud',str(ui.get_page())=='hud')
    yield from shot('world')
    hud.toggle_inventory();yield delay(.4)
    check('inventory_open',str(ui.get_page())=='inventory')
    check('inventory_still_pauses',unreal.GameplayStatics.is_game_paused(w))
    check('inventory_select',ui.execute_action('item:axe'))
    check('inventory_equip',ui.execute_action('use') and str(game.equipment.get('weapon'))=='axe')
    yield from shot('inventory')
    hud.toggle_inventory();check('inventory_close',str(ui.get_page())=='hud' and not unreal.GameplayStatics.is_game_paused(w))
    c=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.HearthwardCompanionFixture)[0]
    check('companion_in_range',p.get_actor_location().distance(c.get_actor_location())<3000)
    hud.toggle_dialogue();yield delay(.4)
    check('dialogue_open',str(ui.get_page())=='dialogue')
    check('dialogue_world_running',not unreal.GameplayStatics.is_game_paused(w))
    draft=next(x for x in unreal.ObjectIterator(unreal.EditableTextBox) if x.get_outer().get_outer()==ui)
    draft.set_text('界面输入检查');check('dialogue_input',str(draft.get_text())=='界面输入检查')
    yield from shot('dialogue')
    # A changed scene behind the same open UI proves this is live world rendering.
    rot=pc.get_control_rotation();pc.set_control_rotation(unreal.Rotator(pitch=rot.pitch,yaw=rot.yaw+55))
    yield delay(.5);yield from shot('dialogue-turned')
    check('dialogue_stays_open',str(ui.get_page())=='dialogue')
    check('memory_open',ui.execute_action('page:memory') and str(ui.get_page())=='memory')
    check('memory_return',ui.execute_action('back') and str(ui.get_page())=='dialogue' and not unreal.GameplayStatics.is_game_paused(w))
    hud.toggle_dialogue();check('dialogue_close',str(ui.get_page())=='hud')
    hud.toggle_inventory();check('inventory_reopen',str(ui.get_page())=='inventory')
    ui.execute_action('back');check('return_controls',str(ui.get_page())=='hud' and not unreal.GameplayStatics.is_game_paused(w))
    report['ok']=True
flow=run();pending=None
def finish():
    (out/'result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle);levels.editor_request_end_play();unreal.SystemLibrary.quit_editor()
def tick(dt):
    global pending
    try:
        if pending:
            pred,end=pending
            if not pred():
                if time.monotonic()>end:raise TimeoutError('UI scene overlay')
                return
        pending=next(flow)
    except StopIteration:finish()
    except Exception:report['error']=traceback.format_exc();finish()
handle=unreal.register_slate_post_tick_callback(tick)
