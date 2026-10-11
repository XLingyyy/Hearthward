"""Unsaved UE comparison: original LOD0 and both reduced LODs at identical cameras."""
import json, os, time, traceback
from pathlib import Path
import unreal

out = Path(os.environ['HEARTHWARD_TASK110_QA'])
frames = out / 'frames'
frames.mkdir(exist_ok=True)
report = {'ok': False, 'fixture': 'unsaved render of actual item meshes and MetalFinish; reference only', 'samples': [], 'textures': []}
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
    for rotation, intensity in [(unreal.Rotator(-35, -45, 0), 60.), (unreal.Rotator(-20, 135, 0), 30.)]:
        light = actors.spawn_actor_from_class(unreal.DirectionalLight, origin, rotation)
        light.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(intensity)
    sky_actor = actors.spawn_actor_from_class(unreal.SkyLight, origin)
    sky = sky_actor.get_component_by_class(unreal.SkyLightComponent)
    sky.set_editor_property('source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.set_cubemap(unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
    sky.set_intensity(2.)
    prop = actors.spawn_actor_from_class(unreal.StaticMeshActor, origin)
    component = prop.static_mesh_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    capture = actors.spawn_actor_from_class(unreal.SceneCapture2D, origin)
    capture.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    target = unreal.RenderingLibrary.create_render_target2d(world, 1024, 1024, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    camera = capture.capture_component2d
    camera.texture_target = target
    camera.capture_source = unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
    camera.set_editor_property('capture_every_frame', False)
    camera.set_editor_property('capture_on_movement', False)
    camera.fov_angle = 48
    settings = camera.get_editor_property('post_process_settings')
    for field, value in [('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL),
                         ('auto_exposure_apply_physical_camera_exposure', True), ('depth_of_field_fstop', 2.),
                         ('camera_shutter_speed', 64.), ('camera_iso', 100.), ('auto_exposure_bias', 0.),
                         ('bloom_intensity', 0.), ('motion_blur_amount', 0.)]:
        settings.set_editor_property('override_' + field, True)
        settings.set_editor_property(field, value)
    camera.set_editor_property('post_process_settings', settings)
    report['lighting'] = 'TASK-070 isolated preview: directional 60/30 lux, daylight cubemap 2; manual f2, 1/64s, ISO100; identical for all LODs'
    paths = [('axe', '/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe', 0)]
    for family in ['Shortblade','Longblade','Spear','Waraxe','Crossbow']:
        for metal in ([0,1] if family in ['Shortblade','Longblade','Spear'] else [1]):
            paths.append((family.lower()+('-metal' if metal else '-stone'),f'/Game/Hearthward/Assets/TASK-095/Weapons/{family}/SM_{family}_Practical',metal))
    for family in ['Longbow','Quiver','Arrow']:
        paths.append((family.lower(),f'/Game/Hearthward/Assets/TASK-095/Archery/{family}/SM_{family}_Practical',None))
    for name,path,metal in paths:
        mesh=unreal.load_asset(path); assert mesh,path
        component.set_static_mesh(mesh)
        component.set_editor_property('override_materials', [])
        if metal is not None and name!='axe':
            material=component.create_dynamic_material_instance(0)
            material.set_scalar_parameter_value('MetalFinish',float(metal))
        component.set_forced_lod_model(1)
        center,extent=prop.get_actor_bounds(False)
        radius=max(extent.x,extent.y,extent.z)
        location=center+unreal.Vector(radius*1.8,radius*3.2,radius*1.4)
        capture.set_actor_location(location,False,True)
        capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,center),False)
        yield delay(2)
        camera.capture_scene()
        yield delay(.3)
        unreal.RenderingLibrary.export_render_target(world,target,str(frames),name+'.png')
        report['samples'].append({'item_visual':name,'mesh':path,'metal_finish':metal,'frame':name+'.png'})
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
    (out / 'references.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)


handle = unreal.register_slate_post_tick_callback(tick)
