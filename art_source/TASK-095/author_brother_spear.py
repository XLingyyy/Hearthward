"""Original two-hand brother thrust: contact at the approved 0.25 seconds."""
import bpy,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir

with bpy.data.libraries.load(str(ROOT/'art_source/TASK-095/Brother.blend'),link=False) as (src,dst):
    dst.scenes=[src.scenes[0]]
scene=dst.scenes[0];scene.name='TASK095_BrotherSpear'
bpy.context.window.scene=scene
rig=next(o for o in scene.objects if o.type=='ARMATURE')
helper=(ROOT/'art_source/TASK-095/author_spear_motion.py').read_text(encoding='utf-8').split('reset();rows=[];errors=[]')[0]
helper=helper[helper.index('REST='):]
from mathutils import Matrix,Vector
import math
rig.data.pose_position='POSE';rig.animation_data_create();rig.animation_data.action=None
exec(compile(helper,'spear_ik_helpers','exec'))
rows=[];errors=[]
for i in range(61):
    t=i/60;reset()
    thrust=smooth((t-.12)/.13) if t<=.25 else 1-smooth((t-.25)/.45)
    pull=smooth(t/.12)*(1-thrust) if t<.25 else 0
    spine=rig.pose.bones['spine_01'];point=spine.head.copy()
    spine.matrix=Matrix.Translation(point)@Matrix.Rotation(math.radians(12*thrust),4,'X')@Matrix.Rotation(math.radians(-60),4,'Z')@Matrix.Translation(-point)@spine.matrix
    bpy.context.view_layer.update()
    head=rig.pose.bones['head'];point=head.head.copy()
    head.matrix=Matrix.Translation(point)@Matrix.Rotation(math.radians(60),4,'Z')@Matrix.Translation(-point)@head.matrix
    bpy.context.view_layer.update()
    y=-.08+.025*pull-.105*thrust
    right=(-.12,y,.68);left=(-.12,y-.12,.68)
    errors.append([hand('r',right),hand('l',left)])
    close_fingers('r',right);close_fingers('l',left)
    rows.append({p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in rig.pose.bones})
assert max(max(e) for e in errors)<.003,errors
action=bpy.data.actions.new('A_Brother_SpearThrust');action.use_fake_user=True;rig.animation_data.action=action
previous={}
for frame,row in enumerate(rows,1):
    for n,(loc,q,scale) in row.items():
        if n in previous and q.dot(previous[n])<0:q.negate()
        previous[n]=q.copy();p=rig.pose.bones[n];p.location=loc;p.rotation_quaternion=q;p.scale=scale
        p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
scene.render.fps=60;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=61;scene.frame_set(1)
out=task_output_dir('Hearthward','motion','TASK-095-brother-spear','20261009');out.mkdir(parents=True,exist_ok=True)
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
fbx=out/'A_Brother_SpearThrust.fbx'
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
with bpy.data.libraries.load(str(ROOT/'art_source/TASK-095/Weapons-Practical.blend'),link=False) as (src,dst):dst.objects=['SM_Spear_Practical']
spear=dst.objects[0];scene.collection.objects.link(spear)
spear.parent=rig;spear.parent_type='BONE';spear.parent_bone='hand_r';spear.matrix_parent_inverse=Matrix.Identity(4)
spear.matrix_basis=Matrix.Translation((0,-rig.data.bones['hand_r'].length,0))@GRIPS['r']@Matrix.Scale(.97863766/1.6,4)
scene.frame_set(16);bpy.context.view_layer.update()
bpy.data.libraries.write(str(ROOT/'art_source/TASK-095/Brother-Spear.blend'),{scene},fake_user=True)
report={'source':'Original brother motion on existing Brother rig; reuses original Hero two-hand IK authoring helpers','seconds':1,'contact_seconds':.25,'fps':60,'grip_errors_source_m':errors,'fbx':str(fbx)}
(out/'author.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
(out/'meta.json').write_text(json.dumps({'animation_fbx_path':str(fbx)},indent=2),encoding='utf-8')
print(json.dumps({'fbx':str(fbx),'max_grip_error':max(max(e) for e in errors),'rig':rig.name}))
