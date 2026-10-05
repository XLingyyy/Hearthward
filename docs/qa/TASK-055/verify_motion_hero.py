"""Candidate asset pose QA on the real Hero mesh; no gameplay acceptance claim."""
import json
import time
import traceback
from pathlib import Path
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
avatar = 'Brother' if 'Task055Avatar=Brother' in unreal.SystemLibrary.get_command_line() else 'Hero'
out = Path(unreal.Paths.project_dir()) / ('Saved/Task055/motion-' + avatar.lower())
out.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {'ok': False, 'scope': 'candidate animation asset pose QA', 'avatar': avatar, 'checks': {}, 'clips': {}}

def wait(pred, seconds=40):
    return pred, time.monotonic() + seconds

def delay(seconds):
    end = time.monotonic() + seconds
    return wait(lambda: time.monotonic() >= end, seconds + 5)

def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()

def check(name, value):
    report['checks'][name] = bool(value)
    if not value:
        raise AssertionError(name)

def pose(mesh):
    return {b: mesh.get_socket_transform(b, unreal.RelativeTransformSpace.RTS_COMPONENT).translation
            for b in ['head', 'hand_l', 'hand_r', 'foot_l', 'foot_r']}

def run():
    light = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.PointLight, unreal.Vector(0, 0, 300))
    light.tags = ['Task055PoseLight']
    light.point_light_component.set_intensity(150)
    light.point_light_component.set_attenuation_radius(1200)
    levels.editor_request_begin_play()
    yield wait(levels.is_in_play_in_editor)
    yield delay(2)
    pc = unreal.GameplayStatics.get_player_controller(world(), 0)
    unreal.GameplayStatics.set_game_paused(world(), False)
    p = unreal.GameplayStatics.get_player_pawn(world(), 0)
    actor = p
    if avatar == 'Brother':
        unreal.SystemLibrary.execute_console_command(world(), 'Hearthward.Companion.CreateTest', pc)
        yield delay(.5)
        actor = unreal.GameplayStatics.get_actor_of_class(world(), unreal.HearthwardCompanionFixture)
    mesh = actor.mesh
    check('actual_' + avatar.lower() + '_mesh', mesh.get_skinned_asset().get_path_name().startswith('/Game/Characters/' + avatar + '/UE5/SK_' + avatar + '.'))
    p.get_component_by_class(unreal.CharacterMovementComponent).disable_movement()
    actor.get_component_by_class(unreal.CharacterMovementComponent).disable_movement()
    p.get_component_by_class(unreal.HearthwardGameplayComponent).set_component_tick_enabled(False)
    p.get_component_by_class(unreal.HearthwardPresentationComponent).set_component_tick_enabled(False)
    pc.get_hud().screen.set_visibility(unreal.SlateVisibility.HIDDEN)
    mesh.set_editor_property('visibility_based_anim_tick_option', unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
    mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    camera = p.get_component_by_class(unreal.CameraComponent)
    camera.set_absolute(True, True, False)
    center = actor.get_actor_location()
    for light_actor in unreal.GameplayStatics.get_all_actors_of_class(world(), unreal.PointLight):
        if light_actor.actor_has_tag('Task055PoseLight'):
            light_actor.set_actor_location(center + unreal.Vector(100, -150, 200), False, False)
    for name in ['slash', 'hit_to_side', 'fall', 'swim']:
        clip = unreal.load_asset('/Game/Hearthward/Assets/TASK-055/Motion/' + avatar + '/knight61_SK_' + avatar + '_Skeleton_Anim' + name)
        check('loaded_' + name, clip is not None)
        check('skeleton_' + name, clip.get_editor_property('skeleton').get_path_name().startswith('/Game/Characters/' + avatar + '/UE5/SK_' + avatar + '_Skeleton.'))
        mesh.play_animation(clip, True)
        mesh.set_play_rate(1.)
        yield delay(.25)
        a = pose(mesh)
        yield delay(.35)
        b = pose(mesh)
        displacement = {k: (b[k] - a[k]).length() for k in a}
        check('animated_' + name, max(displacement.values()) > .05)
        bounds = unreal.SystemLibrary.get_component_bounds(mesh)
        report['clips'][name] = {'length': clip.get_play_length(), 'joint_displacement': displacement, 'component_world_bounds': [str(v) for v in bounds]}
        mesh.set_play_rate(0.)
        mesh.set_position(clip.get_play_length() * .35, False)
        yield delay(.1)
        for view, offset in [('front', actor.get_actor_forward_vector() * 320 + unreal.Vector(0, 0, 15)), ('side', actor.get_actor_right_vector() * 320 + unreal.Vector(0, 0, 15))]:
            eye = center + offset
            camera.set_world_location_and_rotation(eye, unreal.MathLibrary.find_look_at_rotation(eye, center), False, False)
            yield delay(.25)
            target = out / (name + '-' + view + '.png')
            unreal.SystemLibrary.execute_console_command(world(), 'HighResShot 1024x768 filename="' + str(target) + '"', pc)
            yield delay(.5)

def finish(error=None):
    report['ok'] = error is None and bool(report['checks']) and all(report['checks'].values())
    if error:
        report['error'] = error
    (out / 'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()

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
