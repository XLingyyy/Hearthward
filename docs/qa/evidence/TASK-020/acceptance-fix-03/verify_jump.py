"""PIE regression through the actual Enhanced Input action and character movement."""
import json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task020'
baseline=(Path(unreal.Paths.project_dir())/'.agent-local/jump-baseline').exists()
report={'passed':False,'baseline':baseline,'checks':{}}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
state={}
def check(name,value):
    report['checks'][name]=bool(value)
    if not value:raise AssertionError(name)
def delay(seconds):
    end=time.monotonic()+seconds
    return lambda:time.monotonic()>end
def finish():
    (out/'fix03-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
def run():
    levels.editor_request_begin_play();yield levels.is_in_play_in_editor;yield delay(2)
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pc=unreal.GameplayStatics.get_player_controller(world,0);ui=pc.get_hud().screen
    ui.execute_action('new');yield delay(.5)
    pawn=unreal.GameplayStatics.get_player_pawn(world,0)
    actions=[a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_outer()==pawn]
    jump=next((a for a in actions if a.get_name()=='JumpAction'),None)
    if baseline:
        check('reproduced_missing_jump_action',jump is None)
        z=pawn.get_actor_location().z;pawn.jump();yield delay(.2)
        check('engine_jump_already_works',pawn.get_actor_location().z>z+20)
        pawn.stop_jumping();report['passed']=True;finish();return
    check('jump_action_exists',jump is not None)
    sub=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.query_keys_mapped_to_action(jump))
    report['mapped_keys']=[str(unreal.InputLibrary.key_get_display_name(k)) for k in sub.query_keys_mapped_to_action(jump)]
    check('space_mapped',report['mapped_keys'] in (['Space Bar'],['空格'],['空格键'],['Space']))
    state.update(sub=sub,jump=jump,held=False)
    rows=json.loads(ui.describe_layout())['components']
    check('hud_heading_removed',not any(r['id']=='hud.compass' or '°' in r.get('text','') for r in rows))
    check('hud_capture',ui.capture_ui('fix03-hud',1672,941))
    movement=pawn.get_component_by_class(unreal.CharacterMovementComponent)
    yield delay(.3);check('starts_grounded',movement.is_moving_on_ground())
    z=pawn.get_actor_location().z
    action=pawn.get_component_by_class(unreal.HearthwardTimedActionComponent)
    action.start_action();state['held']=True;yield delay(.2)
    check('input_causes_ascent',pawn.get_actor_location().z>z+20 and pawn.get_velocity().z>0)
    check('jump_interrupts_action',action.get_status()!=unreal.HearthwardTimedActionStatus.RUNNING)
    state['held']=False;yield delay(.03)
    check('release_clears_jump',not pawn.get_editor_property('pressed_jump'))
    state['held']=True;yield delay(.15)
    check('no_air_jump',pawn.get_editor_property('jump_current_count')==1)
    yield delay(1.2)
    check('lands_without_auto_repeat',movement.is_moving_on_ground() and abs(pawn.get_actor_location().z-z)<2)
    state['held']=False;yield delay(.1);state['held']=True;yield delay(.15)
    check('can_jump_again_after_landing',pawn.get_actor_location().z>z+20)
    state['held']=False;yield delay(1)
    ui.open_page('pause');p=pawn.get_actor_location();state['held']=True;yield delay(.3)
    check('paused_menu_does_not_jump',(pawn.get_actor_location()-p).length()<1)
    state['held']=False;ui.open_page('hud');yield delay(.2)
    report['passed']=True;finish()
iterator=run();pending=None;deadline=time.monotonic()+90
def tick(delta):
    global pending
    try:
        if time.monotonic()>deadline:raise TimeoutError('jump regression')
        if state.get('held'):state['sub'].inject_input_vector_for_action(state['jump'],unreal.Vector(1,0,0),[],[])
        if pending and not pending():return
        pending=next(iterator)
    except StopIteration:unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc();finish();unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
