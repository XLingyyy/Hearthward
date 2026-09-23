"""Reproduce reported sprint-follow and equipped-attack presentation on TASK-042 natural map."""
import json, math, re, time, traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Task029Integration/reported-playability-after.json'
out.parent.mkdir(parents=True,exist_ok=True)
report={'passed':False,'checks':{},'scope':'isolated PIE, gameplay calls and pawn movement; physical keys/visual pose not covered'}
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

def record(name,value): report['checks'][name]=bool(value)
def wait(fn,seconds=45): return fn,time.monotonic()+seconds
def delay(seconds):
    deadline=time.monotonic()+seconds
    return wait(lambda:time.monotonic()>=deadline)
def game(): return editor.get_game_world()
def screen():
    g=game();pc=unreal.GameplayStatics.get_player_controller(g,0) if g else None
    return pc.get_hud().screen if pc and pc.get_hud() else None

def run():
    record('isolated_pool',bool(re.search(r'-HearthwardSaveTestPool=[0-9a-fA-F-]+',unreal.SystemLibrary.get_command_line())))
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(3)
    record('entry',bool(screen() and screen().execute_action('new')))
    yield wait(lambda:game() and str(unreal.GameplayStatics.get_current_level_name(game(),True))=='L_HearthwardWilds' and screen() and str(screen().get_page())=='hud',120)
    pawn=unreal.GameplayStatics.get_player_pawn(game(),0)
    gp=pawn.get_component_by_class(unreal.HearthwardGameplayComponent)
    bag=pawn.get_component_by_class(unreal.HearthwardInventoryComponent)
    buddy=unreal.GameplayStatics.get_all_actors_of_class(game(),unreal.HearthwardCompanionFixture)[0]
    record('starter_axe',bag.get_item_count('axe')>=1)
    record('equip_axe',gp.equip('axe'))
    report['attack_without_target_result']=gp.attack()
    yield delay(.06)
    mesh=pawn.get_component_by_class(unreal.SkeletalMeshComponent)
    anim=mesh.get_anim_instance() if mesh else None
    report['motion_after_miss']=str(anim.motion_state) if anim else 'none'
    report['attack_feedback']=str(gp.feedback)
    record('equipped_attack_animates_without_target',bool(anim and str(anim.motion_state)=='Attack'))
    record('follow_order_near_companion',gp.order_companion('follow'))
    source=buddy.source.get_owner();point=source.get_actor_location();origin=pawn.get_actor_location()
    dx,dy=point.x-origin.x,point.y-origin.y;length=math.hypot(dx,dy)
    direction=unreal.Vector(dx/length,dy/length,0) if length else unreal.Vector(1,0,0)
    gp.set_sprinting(True)
    end=time.monotonic()+8
    yield wait(lambda:(pawn.add_movement_input(direction,1),time.monotonic()>=end)[1],12)
    report['player_speed']=pawn.get_component_by_class(unreal.CharacterMovementComponent).max_walk_speed
    report['companion_speed']=buddy.get_component_by_class(unreal.CharacterMovementComponent).max_walk_speed
    report['player_moved_cm']=math.hypot(pawn.get_actor_location().x-origin.x,pawn.get_actor_location().y-origin.y)
    report['separation_cm']=math.hypot(pawn.get_actor_location().x-buddy.get_actor_location().x,pawn.get_actor_location().y-buddy.get_actor_location().y)
    record('sprint_chase_can_close_gap',report['companion_speed']>report['player_speed'])
    record('long_sprint_companion_stays_near',report['player_moved_cm']>3000 and report['separation_cm']<500)
    gp.set_sprinting(False)
    report['passed']=all(report['checks'].values())
    out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    levels.editor_request_end_play()
    unreal.SystemLibrary.quit_editor()

it=run();pending=None

def tick(_):
    global pending
    try:
        if pending:
            fn,deadline=pending
            if not fn():
                if time.monotonic()>deadline: raise TimeoutError('reported playability wait')
                return
        pending=next(it)
    except StopIteration:
        unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        report['error']=traceback.format_exc()
        out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        unreal.log_error(report['error'])
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()
        unreal.SystemLibrary.quit_editor()

handle=unreal.register_slate_post_tick_callback(tick)
