"""Transient Hero pose/orientation review; never saves the level."""
import json
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)

template_review='-Task027TemplatePreview' in unreal.SystemLibrary.get_command_line()
out = Path(unreal.Paths.project_saved_dir()) / ('Task027/template-preview' if template_review else 'Task027/preview')
out.mkdir(parents=True, exist_ok=True)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
hero = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 300))
for xyz in [(150, -150, 450), (-150, 150, 450), (150, 150, 400)]:
    light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*xyz))
    light.point_light_component.set_intensity(150 if template_review else 20)
    light.point_light_component.set_attenuation_radius(1000)
mesh = hero.skeletal_mesh_component
mesh.set_skeletal_mesh_asset(unreal.load_asset('/Game/Characters/Hero/Tripo/SK_Hero_Tripo'))
mesh.set_update_animation_in_editor(True)
mesh.set_editor_property('visibility_based_anim_tick_option', unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
mesh.play_animation(unreal.load_asset('/Game/Characters/Hero/Tripo/A_Hero_Tripo_Actionsidle'), True)
capture = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(250, 0, 350))
comp = capture.capture_component2d
target = unreal.RenderingLibrary.create_render_target2d(world, 768, 768, unreal.TextureRenderTargetFormat.RTF_RGBA8)
comp.texture_target = target
comp.capture_source = unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR
comp.set_editor_property('fov_angle', 30)
comp.set_editor_property('capture_every_frame', False)
comp.set_editor_property('capture_on_movement', False)
data = []
for i in range(mesh.get_num_bones()):
    bone = mesh.get_bone_name(i)
    data.append({'bone': str(bone), 'parent': str(mesh.get_parent_bone(bone)),
                 'position': str(mesh.get_socket_location(bone))})
(out/'skeleton.json').write_text(json.dumps(data, indent=2), encoding='utf-8')
views = [('plus_x', (250, 0, 350)), ('minus_x', (-250, 0, 350)), ('plus_y', (0, 250, 350)), ('minus_y', (0, -250, 350))]
if '-Task027MotionPreview' in unreal.SystemLibrary.get_command_line():
    views = [(name+'_'+str(i),(220,-180,365)) for name in ['Idle','Walk','Sprint','JumpStart','Fall','Land','Attack','Dig'] for i in range(4)]
if template_review:
    template_paths=dict(zip(['Idle','Walk','Sprint'],json.loads((out.parent/'template-retarget.json').read_text(encoding='utf-8'))['output']))
    views=[(name+'_'+str(i)+'_'+side,xyz) for name in ['Idle','Walk','Sprint'] for i in range(4) for side,xyz in [('front',(250,0,350)),('side',(0,-250,350))]]
    template_paths['SourceIdle']='/Game/Characters/Hero/Tripo/A_Hero_Tripo_Actionsidle'
    views.append(('SourceIdle_0_front',(250,0,350)))
index = 0
ticks = 0
pending = False
def tick(delta):
    global index, ticks, pending
    ticks += 1
    if ticks < 60 or ticks % 5: return
    if index >= len(views):
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
        return
    name, xyz = views[index]
    if pending == 1:
        comp.capture_scene()
        pending = 2
        return
    if pending == 2:
        unreal.RenderingLibrary.export_render_target(world, target, str(out), name+'.png')
        index += 1
        pending = False
        return
    location = unreal.Vector(*xyz)
    if '-Task027MotionPreview' in unreal.SystemLibrary.get_command_line() or template_review:
        clip_name, sample = name.split('_')[:2]
        clip = unreal.load_asset(template_paths[clip_name] if template_review else '/Game/Characters/Hero/Animation/A_Hero_'+clip_name)
        mesh.play_animation(clip, False)
        mesh.set_play_rate(0.)
        mesh.set_position(clip.get_play_length() * int(sample) / 4, False)
    capture.set_actor_location(location, False, False)
    capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location, unreal.Vector(0, 0, 350)), False)
    pending = 1
handle = unreal.register_slate_post_tick_callback(tick)
