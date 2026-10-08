"""Original two-handed thrust on the existing Hero rig; normalized 0.6/0.2/0.2 phases."""
import bpy,math,json,sys
from pathlib import Path
from mathutils import Matrix,Vector
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir
assert Path(bpy.data.filepath).stem=='Hero'
scene=bpy.context.scene;rig=bpy.data.objects['Armature'];rig.data.pose_position='POSE';rig.animation_data_create();rig.animation_data.action=None
REST={b.name:b.matrix_local.copy() for b in rig.data.bones}
def reset():
 for p in rig.pose.bones:p.rotation_mode='QUATERNION';p.matrix_basis=Matrix.Identity(4)
 bpy.context.view_layer.update()
def axes(direction):
 z=direction.normalized();x=Vector((0,0,1));x=(x-z*x.dot(z)).normalized();y=z.cross(x)
 return Matrix((x,y,z)).transposed().to_4x4()
GRIPS={}
for side in ['r','l']:
 grip=(rig.data.bones['thumb_02_'+side].head_local+rig.data.bones['middle_02_'+side].head_local)*.5
 direction=rig.data.bones['index_01_'+side].head_local-rig.data.bones['pinky_01_'+side].head_local
 weapon=axes(direction);weapon.translation=grip;GRIPS[side]=REST['hand_'+side].inverted()@weapon
def aim(bone,child,point):
 p=rig.pose.bones[bone];head=p.head.copy();direction=rig.pose.bones[child].head-head
 q=direction.normalized().rotation_difference((point-head).normalized())
 p.matrix=Matrix.Translation(head)@q.to_matrix().to_4x4()@Matrix.Translation(-head)@p.matrix
 bpy.context.view_layer.update()
def hand(side,grip):
 weapon=axes(Vector((0,-1,0)));weapon.translation=Vector(grip)
 desired=weapon@GRIPS[side].inverted();point=desired.translation
 upper='upperarm_'+side;lower='lowerarm_'+side;end='hand_'+side
 shoulder=rig.pose.bones[upper].head.copy();elbow=rig.pose.bones[lower].head.copy();wrist=rig.pose.bones[end].head.copy()
 a=(elbow-shoulder).length;b=(wrist-elbow).length;d=point-shoulder
 distance=min(a+b-.001,max(abs(a-b)+.001,d.length));axis=d.normalized()
 pole=Vector((.4 if side=='l' else -.4,.05,-.3));pole=(pole-axis*pole.dot(axis)).normalized()
 along=(a*a+distance*distance-b*b)/(2*distance);height=math.sqrt(max(0,a*a-along*along))
 aim(upper,lower,shoulder+axis*along+pole*height);aim(lower,end,shoulder+axis*distance)
 desired.translation=rig.pose.bones[end].head.copy();rig.pose.bones[end].matrix=desired;bpy.context.view_layer.update()
 actual=rig.pose.bones[end].matrix@GRIPS[side]
 return (actual.translation-Vector(grip)).length
def smooth(t):t=max(0,min(1,t));return t*t*(3-2*t)
def close_fingers(side,grip):
 for finger in ['index','middle','ring','pinky']:
  for joint in ['02','03']:
   p=rig.pose.bones[finger+'_'+joint+'_'+side];head=p.head.copy()
   closest=Vector((grip[0],head.y,grip[2]));radial=head-closest
   point=closest+radial.normalized()*.008
   q=(p.tail-head).normalized().rotation_difference((point-head).normalized())
   if q.angle>math.radians(70):q=q.slerp(type(q)(),1-math.radians(70)/q.angle)
   p.matrix=Matrix.Translation(head)@q.to_matrix().to_4x4()@Matrix.Translation(-head)@p.matrix
   bpy.context.view_layer.update()
reset();rows=[];errors=[]
for i in range(31):
 t=i/30;reset()
 thrust=smooth((t-.6)/.2) if t<=.8 else 1-smooth((t-.8)/.2)
 pull=smooth(t/.6)*(1-thrust) if t<.8 else 0
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
assert max(max(e) for e in errors)<.003,max(max(e) for e in errors)
action=bpy.data.actions.new('A_Hero_SpearThrust');action.use_fake_user=True;rig.animation_data.action=action
previous={}
for frame,row in enumerate(rows,1):
 for n,(loc,q,scale) in row.items():
  if n in previous and q.dot(previous[n])<0:q.negate()
  previous[n]=q.copy();p=rig.pose.bones[n];p.location=loc;p.rotation_quaternion=q;p.scale=scale
  p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
scene.render.fps=30;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=31;scene.frame_set(1)
out=task_output_dir('Hearthward','motion','TASK-095-spear','weapons-20261008');out.mkdir(parents=True,exist_ok=True)
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(out/(action.name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
with bpy.data.libraries.load(str(ROOT/'art_source/TASK-095/Weapons-Practical.blend'),link=False) as (src,dst):dst.objects=['SM_Spear_Practical']
spear=dst.objects[0];bpy.context.collection.objects.link(spear)
# Preview the same hand-relative attachment used in the engine; source character is 0.9787 m for 1.8 m in game.
spear.parent=rig;spear.parent_type='BONE';spear.parent_bone='hand_r'
spear.matrix_parent_inverse=Matrix.Identity(4)
spear.matrix_basis=Matrix.Translation((0,-rig.data.bones['hand_r'].length,0))@GRIPS['r']@Matrix.Scale(.9786989/1.8,4)
scene.frame_set(1);bpy.context.view_layer.update()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/Hero-Spear.blend'))
report={'source':'Original two-handed spear thrust','seconds':1,'normalized_phase_boundaries':[.6,.8,1],'grip_errors_source_m':errors,'fbx':str(out/(action.name+'.fbx'))}
(out/'author.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print('SPEAR_AUTHORED',max(max(e) for e in errors))
