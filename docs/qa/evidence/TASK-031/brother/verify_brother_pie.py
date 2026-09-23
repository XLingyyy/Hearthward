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
out = root / 'Saved/BrotherValidation/playtest4'
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

def pose(mesh):
    return {bone:mesh.get_socket_transform(bone,unreal.RelativeTransformSpace.RTS_COMPONENT).translation for bone in ['hand_l','hand_r','foot_l','foot_r']}

def shot(name,c):
    w=world();pc=unreal.GameplayStatics.get_player_controller(w,0)
    p=unreal.GameplayStatics.get_player_pawn(w,0);cam=p.get_component_by_class(unreal.CameraComponent)
    cam.set_absolute(True,True,False)
    loc=c.get_actor_location()+c.get_actor_forward_vector()*280+c.get_actor_right_vector()*180+unreal.Vector(0,0,25)
    cam.set_world_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,c.get_actor_location()),False,False)
    yield delay(.3)
    unreal.SystemLibrary.execute_console_command(w,'HighResShot 1024x768 filename="'+str(out/(name+'.png'))+'"',pc)
    yield delay(.5)


def run():
    levels.editor_request_begin_play();yield wait(levels.is_in_play_in_editor,30);yield delay(2)
    unreal.GameplayStatics.get_player_controller(world(),0).get_hud().screen.execute_action('new')
    yield wait(lambda:unreal.GameplayStatics.get_current_level_name(world(),True)=='L_HearthwardWilds');yield delay(8)
    p=unreal.GameplayStatics.get_player_pawn(world(),0)
    c=unreal.GameplayStatics.get_all_actors_of_class(world(),unreal.HearthwardCompanionFixture)[0]
    mesh=c.mesh;anim=mesh.get_anim_instance();g=p.get_component_by_class(unreal.HearthwardGameplayComponent)
    ai=sub(unreal.HearthwardLocalAISubsystem);store=sub(unreal.HearthwardStorageSubsystem)
    check('brother_mesh',mesh.get_skinned_asset().get_path_name().startswith('/Game/Characters/Brother/UE5/SK_Brother.'))
    check('brother_anim_instance',isinstance(anim,unreal.HearthwardBrotherAnimInstance))
    check('complete_legs',all(mesh.get_bone_index(b)>=0 for b in ['thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r']))
    report['actual_material']=mesh.get_material(0).get_path_name()
    check('pbr_material',mesh.get_material(0).get_name()=='M_Brother')
    check('wait_command',g.order_companion('wait'));yield delay(.5)
    check('wait_animation',str(anim.motion_state)=='Wait')
    yield from shot('wait',c)
    goal=unreal.HearthwardAgentGoal()
    for key,value in dict(intent='collect',item='wood',quantity=2,quantity_mode='additional_acquired',source_ref='S1').items():goal.set_editor_property(key,value)
    check('request',ai.set_structured_goal(p,c,goal));check('confirm',ai.confirm_candidate(ai.get_candidate_id()))
    yield wait(lambda:anim.ground_speed>100,15)
    check('walking_animation',str(anim.motion_state) in ['Walk','Run'])
    a=pose(mesh);yield delay(.3);b=pose(mesh)
    report['walk_joint_displacement']={k:(b[k]-a[k]).length() for k in a}
    check('both_legs_animate',all(report['walk_joint_displacement'][k]>.1 for k in ['foot_l','foot_r']))
    yield from shot('walking',c)
    yield wait(lambda:str(anim.motion_state)=='Dig',30)
    a=pose(mesh);yield delay(.4);b=pose(mesh)
    report['gather_joint_displacement']={k:(b[k]-a[k]).length() for k in a}
    check('gather_pose_changes',max(report['gather_joint_displacement'][k] for k in ['hand_l','hand_r'])>.1)
    yield from shot('gathering',c)
    yield wait(lambda:c.get_delivered()==2,40)
    check('delivered_with_mesh',store.get_item_count('wood')==2 and c.source.get_item_count('wood')==14)
    check('wait_after_delivery',g.order_companion('wait'));yield delay(.5)
    check('work_animation_stopped',str(anim.motion_state)=='Wait')
    yield from shot('returned',c)
    snapshot('complete',c,ai)
    # Exercise all supplied clips on the live skeletal component for deformation review.
    mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    report['clip_checks']={}
    for name in ['Idle','Walk','Run','Dig','Chop','Wait','Jump','Climb']:
        clip=unreal.load_asset('/Game/Characters/Brother/Animation/A_Brother_'+name)
        mesh.play_animation(clip,True);yield delay(.2)
        a=pose(mesh);yield delay(.5);b=pose(mesh)
        changed=max((b[k]-a[k]).length() for k in a)
        report['clip_checks'][name]=changed
        check('clip_animated_'+name,changed>.05)
        yield from shot('clip_'+name,c)
    mesh.set_anim_instance_class(unreal.HearthwardBrotherAnimInstance)
    yield delay(.3)


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
