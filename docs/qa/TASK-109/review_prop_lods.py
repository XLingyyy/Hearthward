"""Unsaved UE comparison: original LOD0 and both reduced LODs at identical cameras."""
import json, os, time, traceback
from pathlib import Path
import unreal

out = Path(os.environ['HEARTHWARD_TASK109_QA'])
frames = out / 'frames'
frames.mkdir(exist_ok=True)
report = {'ok': False, 'fixture': 'unsaved editor mesh comparison; no gameplay/performance credit', 'samples': [], 'textures': []}
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
pending = None


def delay(seconds):
    deadline = time.monotonic() + seconds
    return lambda: time.monotonic() >= deadline


def run():
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    assert world
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    origin = unreal.Vector(0, 0, 0)
    for rotation, intensity in [(unreal.Rotator(-35, -45, 0), 30.), (unreal.Rotator(-20, 135, 0), 10.)]:
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, origin, rotation)
        light.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(intensity)
    sky_actor = actors.spawn_actor_from_class(unreal.SkyLight, origin)
    sky = sky_actor.get_component_by_class(unreal.SkyLightComponent)
    sky.set_editor_property('source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.set_cubemap(unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
    sky.set_intensity(1.)
    prop = actors.spawn_actor_from_class(unreal.StaticMeshActor, origin)
    component = prop.static_mesh_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    capture = actors.spawn_actor_from_class(unreal.SceneCapture2D, origin)
    capture.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1280, 1000, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    camera = capture.capture_component2d
    camera.texture_target = target
    camera.capture_source = unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
    camera.set_editor_property('capture_every_frame', False)
    camera.set_editor_property('capture_on_movement', False)
    camera.fov_angle = 48
    settings = camera.get_editor_property('post_process_settings')
    for field, value in [('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL),
                         ('auto_exposure_apply_physical_camera_exposure', True), ('depth_of_field_fstop', 4.),
                         ('camera_shutter_speed', 64.), ('camera_iso', 100.), ('auto_exposure_bias', 0.),
                         ('bloom_intensity', 0.), ('motion_blur_amount', 0.)]:
        settings.set_editor_property('override_' + field, True)
        settings.set_editor_property(field, value)
    camera.set_editor_property('post_process_settings', settings)
    report['lighting'] = 'TASK-070 isolated preview: directional 30/10 lux, daylight cubemap 1; manual f4, 1/64s, ISO100; identical for all LODs'
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    seen = set()
    for name, path in [
        ('axe', '/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe'),
        ('chair', '/Game/Hearthward/Assets/TASK-028/furniture/wood_chair/SM_wood_chair')]:
        mesh = unreal.load_asset(path)
        assert mesh
        component.set_static_mesh(mesh)
        center, extent = prop.get_actor_bounds(False)
        radius = max(extent.x, extent.y, extent.z)
        assert radius > 0
        for distance in [1., 4.]:
            location = center + unreal.Vector(radius * 3.5 * distance, radius * 3.5 * distance, radius * 1.8 * distance)
            capture.set_actor_location(location, False, True)
            capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, center), False)
            for lod in range(3):
                component.set_forced_lod_model(lod + 1)
                yield delay(2)
                camera.capture_scene()
                yield delay(.2)
                filename = f'{name}-distance-{distance:g}-lod-{lod}.png'
                unreal.RenderingLibrary.export_render_target(world, target, str(frames), filename)
                report['samples'].append({'mesh': path, 'lod': lod, 'triangles': mesh.get_num_triangles(lod), 'distance_factor': distance, 'frame': filename})
        packages = [str(slot.material_interface.get_path_name()).split('.')[0] for slot in mesh.static_materials]
        while packages:
            package = packages.pop()
            if package in seen or not package.startswith('/Game/'):
                continue
            seen.add(package)
            asset = unreal.load_asset(package)
            if isinstance(asset, unreal.Texture2D):
                unreal.SystemLibrary.execute_console_command(world, 'Editor.AsyncTextureCompilationFinishAll')
                report['textures'].append({'path': package, 'size': [asset.blueprint_get_size_x(), asset.blueprint_get_size_y()],
                    'never_stream': asset.get_editor_property('never_stream'), 'lod_bias': asset.get_editor_property('lod_bias'),
                    'compression': str(asset.get_editor_property('compression_settings'))})
            else:
                packages.extend(str(p) for p in registry.get_dependencies(package, options))
    report['ok'] = True


iterator = run()
deadline = time.monotonic() + 240


def tick(delta):
    global pending
    try:
        if time.monotonic() > deadline:
            raise TimeoutError('LOD comparison')
        if pending and not pending():
            return
        pending = next(iterator)
        return
    except StopIteration:
        pass
    except Exception:
        report['error'] = traceback.format_exc()
    (out / 'review.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)


handle = unreal.register_slate_post_tick_callback(tick)
