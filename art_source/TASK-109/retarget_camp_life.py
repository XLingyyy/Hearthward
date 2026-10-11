"""Blender: retarget the CC0 UAL seated clips onto the existing Brother/civilian rig."""
import bpy
import json
import math
import os
import sys
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[2]
sys.path.extend([str(ROOT.parent), str(ROOT.parent / '.venv/Lib/site-packages')])
from operators.gen_motion.funcs.retarget_utils import world_delta as w
from pipeline.common.paths import task_output_dir

scene = bpy.context.scene
assert Path(bpy.data.filepath).name == 'Brother.blend'
target = bpy.data.objects['Armature']
target.data.pose_position = 'POSE'
body = next(o for o in bpy.data.objects if o.type == 'MESH')
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=str(ROOT / 'art_source/TASK-109/UAL1_Standard.glb'))
source = next(o for o in bpy.data.objects if o not in before and o.type == 'ARMATURE')
for obj in set(bpy.data.objects) - before:
    obj.hide_render = True
for track in source.animation_data.nla_tracks:
    track.mute = True
source.animation_data.action = bpy.data.actions['A_TPose']
source.animation_data.action_slot = source.animation_data.action.slots[0]
target.animation_data_clear()
target.animation_data_create()
w.reset_target_pose(target)
scene.frame_set(0)
bpy.context.view_layer.update()
sref = {p.name: (source.matrix_world @ p.matrix).to_quaternion() for p in source.pose.bones}
local = w.cache_local_rest_quaternions(target)
hip = target.pose.bones['pelvis'].head.copy()
source_hip = source.pose.bones['pelvis'].head.copy()
scale = (hip.z - target.pose.bones['foot_l'].head.z) / (source_hip.z - source.pose.bones['foot_l'].head.z)

# Match the source T pose by bone directions without changing the original bind pose.
for side in ('l', 'r'):
    for bone, child in [('upperarm_', 'lowerarm_'), ('lowerarm_', 'hand_')]:
        p = target.pose.bones[bone + side]
        direction = target.pose.bones[child + side].head - p.head
        desired = source.pose.bones[child + side].head - source.pose.bones[bone + side].head
        rotation = direction.normalized().rotation_difference(desired.normalized())
        mat = p.matrix.copy()
        p.matrix = Matrix.Translation(mat.translation) @ rotation.to_matrix().to_4x4() @ Matrix.Translation(-mat.translation) @ mat
        bpy.context.view_layer.update()
tref = {p.name: (target.matrix_world @ p.matrix).to_quaternion() for p in target.pose.bones}
mapping = {p.name: p.name.lower() if p.name == 'Head' else p.name for p in source.pose.bones
           if (p.name.lower() if p.name == 'Head' else p.name) in target.pose.bones}
ordered = w.order_mapping_parent_first(mapping, target)
out = task_output_dir('Hearthward', 'motion', 'TASK-109-camp-life', 'camp-life-20261011')
out.mkdir(parents=True, exist_ok=True)
report = {'source': 'Quaternius Universal Animation Library Standard', 'license': 'CC0', 'mapping': mapping, 'clips': []}
source_fps = scene.render.fps / scene.render.fps_base
for clip, name in [('Sitting_Enter', 'SitDown'), ('Sitting_Idle_Loop', 'SeatedIdle'),
                   ('Sitting_Talking_Loop', 'SeatedTalk'), ('Sitting_Exit', 'StandUp')]:
    original = bpy.data.actions[clip]
    source.animation_data.action = original
    source.animation_data.action_slot = original.slots[0]
    start, end = original.frame_range
    count = round((end - start) / source_fps * 30) + 1
    rows = []
    for index in range(count):
        time = start + index / 30 * source_fps
        scene.frame_set(math.floor(time), subframe=time % 1)
        w.reset_target_pose(target)
        bpy.context.view_layer.update()
        w.retarget_frame(source, target, ordered, sref, tref, local, 0)
        pelvis = target.pose.bones['pelvis']
        mat = pelvis.matrix.copy()
        mat.translation = hip + (source.pose.bones['pelvis'].head - source_hip) * scale
        pelvis.matrix = mat
        bpy.context.view_layer.update()
        obj = body.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = obj.to_mesh()
        floor = min((obj.matrix_world @ v.co).z for v in mesh.vertices)
        obj.to_mesh_clear()
        root = target.pose.bones['root']
        mat = root.matrix.copy()
        mat.translation += target.matrix_world.inverted().to_3x3() @ Vector((0, 0, -floor))
        root.matrix = mat
        bpy.context.view_layer.update()
        rows.append({p.name: (p.location.copy(), p.rotation_quaternion.copy(), p.scale.copy()) for p in target.pose.bones})
    action = bpy.data.actions.new('A_Brother_' + name)
    action.use_fake_user = True
    target.animation_data.action = action
    previous = {}
    for frame, row in enumerate(rows, 1):
        for bone, (location, rotation, scale_vector) in row.items():
            p = target.pose.bones[bone]
            p.location = location
            p.rotation_quaternion = w.stabilize(rotation, previous.get(bone))
            previous[bone] = p.rotation_quaternion.copy()
            p.scale = scale_vector
            for channel in ('location', 'rotation_quaternion', 'scale'):
                p.keyframe_insert(channel, frame=frame)
    scene.render.fps = 30
    scene.render.fps_base = 1
    scene.frame_start = 1
    scene.frame_end = count
    scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT')
    target.select_set(True)
    bpy.context.view_layer.objects.active = target
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    bpy.ops.export_scene.fbx(filepath=str(out / (action.name + '.fbx')), use_selection=True,
        object_types={'ARMATURE'}, add_leaf_bones=False, use_armature_deform_only=False,
        axis_forward='X', axis_up='Z', apply_scale_options='FBX_SCALE_NONE', bake_anim=True,
        bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False, bake_anim_use_all_actions=False,
        bake_anim_force_startend_keying=True, bake_anim_step=1, bake_anim_simplify_factor=0)
    report['clips'].append({'name': action.name, 'frames': count, 'seconds': (count - 1) / 30})
    target.animation_data.action = None
source.hide_set(True)
target.animation_data.action = bpy.data.actions['A_Brother_SeatedIdle']
scene.frame_start = 1
scene.frame_end = 51
scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(out / 'Brother-CampLife.blend'))
(out / 'retarget.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('CAMP_LIFE_OUTPUT', out)
