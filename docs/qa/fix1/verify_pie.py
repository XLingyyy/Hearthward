import json
import time
import traceback
from pathlib import Path

import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out = Path(unreal.Paths.project_saved_dir()) / 'Fix1Reference'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {'ok': False, 'checks': {}}


def check(name, value):
    report['checks'][name] = bool(value)
    if not value:
        raise AssertionError(name)


def delay(seconds):
    end = time.monotonic() + seconds
    return lambda: time.monotonic() >= end


def shot(world, pc, name):
    unreal.SystemLibrary.execute_console_command(world, f'Shot SHOWUI filename="{(out / name).as_posix()}.png" -nosuffix', pc)


def finish(error=None):
    if error:
        report['error'] = error
    report['ok'] = not error and all(report['checks'].values())
    (out / 'verify.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()


def run():
    ew = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ew.get_world_settings().set_editor_property('default_game_mode', unreal.load_class(None, '/Script/Hearthward.HearthwardGameMode'))
    report['day_lights'] = [(a.get_name(), c.intensity, str(c.get_editor_property('light_color'))) for a in unreal.GameplayStatics.get_all_actors_of_class(ew, unreal.DirectionalLight) for c in a.get_components_by_class(unreal.DirectionalLightComponent)]
    levels.editor_request_begin_play()
    yield levels.is_in_play_in_editor
    yield delay(8)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    ui = pc.get_hud().get_editor_property('screen')
    check('new game', ui.execute_action('new'))
    campaign = next(x for x in unreal.ObjectIterator(unreal.HearthwardCampaignSubsystem) if x.get_outer() == world)
    yield lambda: not campaign.busy()
    yield delay(3)
    ui.open_page('hud')
    house = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardTask028CampHouse) if 'CampaignPrologueHouse' in [str(t) for t in a.tags])
    liners = [c for c in house.get_components_by_class(unreal.StaticMeshComponent) if 'RoofLiner' in c.get_name()]
    report['roof'] = [(c.get_name(), str(c.get_collision_enabled()), str(c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA))) for c in liners]
    check('roof camera blockers', len(liners) == 2 and all(c.get_collision_enabled() == unreal.CollisionEnabled.QUERY_ONLY and c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA) == unreal.CollisionResponseType.ECR_BLOCK for c in liners))
    pc.set_control_rotation(unreal.Rotator(pitch=-65, yaw=0))
    yield delay(1)
    camera = pawn.get_component_by_class(unreal.CameraComponent)
    report['camera_z'] = camera.get_world_location().z
    report['roof_z'] = house.get_actor_location().z + 310
    check('look-down camera below roof', camera.get_world_location().z < house.get_actor_location().z + 335)
    mesh = pawn.get_editor_property('mesh')
    axe = next(c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'HeldAxe')
    report['hand'] = str(mesh.get_socket_location('hand_r'))
    report['axe'] = str(axe.get_world_location())
    report['axe_scale'] = str(axe.get_world_scale())
    check('axe attached to right hand', axe.get_attach_parent() == mesh and str(axe.get_attach_socket_name()) == 'hand_r')
    check('axe at hand', (axe.get_world_location() - mesh.get_socket_location('hand_r')).length() < 18)
    check('axe usable scale', axe.get_world_scale().length() < 2)
    shot(world, pc, 'fixed-inside')
    yield delay(1)
    pawn.set_actor_location(house.get_actor_location() + unreal.Vector(700, -136, 120), False, True)
    pc.set_control_rotation(unreal.Rotator(pitch=-12, yaw=180))
    yield delay(2)
    shot(world, pc, 'fixed-outside-house')
    yield delay(1)
    pc.set_control_rotation(unreal.Rotator(pitch=-12, yaw=0))
    yield delay(1)
    shot(world, pc, 'fixed-outside-landscape')
    lights = [(a, c) for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight) for c in a.get_components_by_class(unreal.DirectionalLightComponent)]
    report['night_lights'] = [(a.get_name(), c.intensity, str(c.get_editor_property('light_color')), bool(c.get_editor_property('atmosphere_sun_light'))) for a, c in lights]
    check('moonlight separated from sky sun', len(lights) == 2 and any('CampaignMoonlight' in [str(t) for t in a.tags] and c.intensity == 1.5 and not c.get_editor_property('atmosphere_sun_light') for a, c in lights))
    yield delay(1)
    combat = pawn.get_component_by_class(unreal.HearthwardCombatComponent)
    enemies = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.HearthwardCampaignActor) if a.get_component_by_class(unreal.HearthwardCombatTargetComponent) and a.get_component_by_class(unreal.HearthwardCombatTargetComponent).health > 0]
    report['enemies_loaded'] = len(enemies)
    check('prologue enemies available', len(enemies) >= 1)
    target = enemies[0]
    for other in enemies[1:]:
        other.get_component_by_class(unreal.HearthwardCombatTargetComponent).health = 0
    target.get_controller().stop_movement()
    target.get_controller().set_actor_tick_enabled(False)
    target.get_movement_component().stop_movement_immediately()
    target.set_actor_location(pawn.get_actor_location() + unreal.Vector(110, 0, 0), False, True)
    target.set_actor_rotation(unreal.Rotator(yaw=0), False)
    pc.set_control_rotation(unreal.Rotator(pitch=-10, yaw=0))
    yield delay(.25)
    report['can_execute'] = combat.can_execute()
    report['target_distance'] = (target.get_actor_location() - pawn.get_actor_location()).length()
    report['target_position'] = str(target.get_actor_location())
    report['pawn_position'] = str(pawn.get_actor_location())
    report['target_movement'] = str(target.get_movement_component().movement_mode)
    check('execution cue is contextual', combat.can_execute())
    shot(world, pc, 'fixed-execution-cue')
    check('R stun starts', combat.stun(target))
    report['stun_description'] = combat.describe()
    check('R stun label', '击晕' in combat.describe())
    yield lambda: not combat.busy()
    target_state = target.get_component_by_class(unreal.HearthwardCombatTargetComponent)
    report['target_health'] = target_state.health
    report['target_awareness'] = str(target_state.awareness)
    check('R stun clears enemy', target_state.health == 0 and str(target_state.awareness) == '已击晕')
    finish()


runner = run()
pending = None
deadline = time.monotonic() + 210


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError('fix1 verification')
        if pending and not pending():
            return
        pending = next(runner)
    except StopIteration:
        pass
    except Exception:
        finish(traceback.format_exc())


handle = unreal.register_slate_post_tick_callback(tick)
