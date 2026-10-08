"""Original right-arm carry pose; preserves the existing Brother skeleton."""
from pathlib import Path
import bpy, json, math, sys
from mathutils import Matrix, Vector
ROOT=Path(__file__).resolve().parents[2]
# Reuse the existing original pose author's two-bone IK and palm basis.
helper=(ROOT/'art_source/TASK-095/author_player_ranged.py').read_text(encoding='utf-8').split('# Match the left-hand')[0]
exec(compile(helper.replace("stem=='Hero'", "stem=='Brother'"),str(__file__),'exec'))
reset()
grip=Vector((-.22,-.13,.57))
error=hand('r',grip,axes(Vector((0,-.25,1))))
assert error<.003,error
for finger in ['index','middle','ring','pinky']:
 for joint in ['02','03']:
  p=rig.pose.bones[finger+'_'+joint+'_r'];head=p.head.copy()
  axis=Vector((0,-.25,1)).normalized();closest=grip+axis*(head-grip).dot(axis)
  target=closest+(head-closest).normalized()*.008
  q=(p.tail-head).normalized().rotation_difference((target-head).normalized())
  if q.angle>math.radians(65):q=q.slerp(type(q)(),1-math.radians(65)/q.angle)
  p.matrix=Matrix.Translation(head)@q.to_matrix().to_4x4()@Matrix.Translation(-head)@p.matrix
  bpy.context.view_layer.update()
action=bpy.data.actions.new('A_Brother_WeaponCarry');action.use_fake_user=True;rig.animation_data.action=action
for frame in [1,61]:
 for p in rig.pose.bones:
  p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
scene.render.fps=60;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=61;scene.frame_set(1)
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
out=task_output_dir('Hearthward','motion','TASK-095-brother-carry','brother-carry-20261008');out.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(out/(action.name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/Brother-WeaponCarry.blend'))
report={'source':'Original authored palm/arm carry pose; existing Brother rig unchanged','clip':action.name,'maximum_grip_error_source_m':error,'seconds':1,'layer':'upperarm_r descendants only; locomotion remains original'}
(out/'author.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report))
