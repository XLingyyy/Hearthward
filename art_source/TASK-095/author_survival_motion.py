"""Retarget CMU 113_08 and author paired rescue poses on the original character rigs."""
import bpy,sys,math,json
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(__file__).resolve().parents[2]
sys.path.extend([str(ROOT.parent),str(ROOT.parent/'.venv/Lib/site-packages')])
from operators.gen_motion.funcs.retarget_utils import world_delta as w,mapping_presets as m
scene=bpy.context.scene
role=Path(bpy.data.filepath).stem
assert role in ['Hero','Brother']
bpy.ops.import_anim.bvh(filepath=str(ROOT/'art_source/TASK-095/cmu/113_08.bvh'),axis_forward='-Z',axis_up='Y',global_scale=.0254,use_fps_scale=False,update_scene_fps=True)
source=bpy.context.object;source.name='CMU_113_08';source.hide_render=True
target=bpy.data.objects['Armature']
body=next(o for o in bpy.data.objects if o.type=='MESH')
source.rotation_euler=(0,0,0);target.data.pose_position='POSE'
target.animation_data_create();target.animation_data.action=None
w.reset_target_pose(target);scene.frame_set(1);bpy.context.view_layer.update()
sref={p.name:(source.matrix_world@p.matrix).to_quaternion() for p in source.pose.bones}
local=w.cache_local_rest_quaternions(target)
resthip=(target.matrix_world@target.pose.bones['pelvis'].matrix).translation.copy()
sourceleg=(source.pose.bones['Hips'].head-source.pose.bones['LeftFoot'].head).z
targetleg=(target.pose.bones['pelvis'].head-target.pose.bones['foot_l'].head).z
scale=targetleg/sourceleg
for side,srcside,sign in [('l','Left',1),('r','Right',-1)]:
 for bone,child,sbone,schild in [('upperarm_','lowerarm_','Arm','ForeArm'),('lowerarm_','hand_','ForeArm','Hand'),('hand_',None,'ForeArm','Hand'),('thigh_','calf_','UpLeg','Leg'),('calf_','foot_','Leg','Foot'),('foot_','ball_','Foot','ToeBase')]:
  p=target.pose.bones[bone+side]
  direction=(target.pose.bones[child+side].head-p.head) if child else (p.tail-p.head)
  desired=source.pose.bones[srcside+schild].head-source.pose.bones[srcside+sbone].head
  rotation=direction.normalized().rotation_difference(desired.normalized())
  mat=p.matrix.copy();mat=Matrix.Translation(mat.translation)@rotation.to_matrix().to_4x4()@Matrix.Translation(-mat.translation)@mat
  p.matrix=mat;bpy.context.view_layer.update()
tref={p.name:(target.matrix_world@p.matrix).to_quaternion() for p in target.pose.bones}
sm=m.SOURCE_SKELETONS['cmu_bvh'].bones;tm=m.SOURCE_SKELETONS['ue5_mannequin'].bones
mapping={sm[k]:tm[k] for k in sm if k in tm};ordered=w.order_mapping_parent_first(mapping,target)
scene.frame_set(1800)
delta=(source.matrix_world@source.pose.bones['Hips'].matrix).to_quaternion()@sref['Hips'].inverted()
forward=delta@Vector((0,-1,0));yaw=math.atan2(forward.x,-forward.y)
source.rotation_euler.z=-yaw;bpy.context.view_layer.update()
endhip=(source.matrix_world@source.pose.bones['Hips'].matrix).translation.copy()
def capture(frame):
 scene.frame_set(frame);w.reset_target_pose(target);bpy.context.view_layer.update()
 w.retarget_frame(source,target,ordered,sref,tref,local,0)
 p=target.pose.bones['root'];mat=p.matrix.copy()
 hp=(source.matrix_world@source.pose.bones['Hips'].matrix).translation
 mat.translation=target.matrix_world.inverted()@((hp-endhip)*scale);p.matrix=mat;bpy.context.view_layer.update()
 dg=bpy.context.evaluated_depsgraph_get();obj=body.evaluated_get(dg);mesh=obj.to_mesh()
 floor=min((obj.matrix_world@v.co).z for v in mesh.vertices);obj.to_mesh_clear()
 mat=p.matrix.copy();mat.translation+=target.matrix_world.inverted().to_3x3()@Vector((0,0,-floor));p.matrix=mat;bpy.context.view_layer.update()
 return {p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in target.pose.bones}
def smooth(t):
 t=max(0,min(1,t));return t*t*(3-2*t)
def mix(a,b,t):
 return {n:(a[n][0].lerp(b[n][0],t),a[n][1].slerp(b[n][1],t),a[n][2].lerp(b[n][2],t)) for n in a}
def apply(row):
 for n,(v,q,s) in row.items():
  p=target.pose.bones[n];p.location=v;p.rotation_quaternion=q;p.scale=s
 bpy.context.view_layer.update()
def aim(bone,child,point):
 p=target.pose.bones[bone];direction=target.pose.bones[child].head-p.head
 rotation=direction.normalized().rotation_difference((point-p.head).normalized())
 mat=p.matrix.copy();p.matrix=Matrix.Translation(mat.translation)@rotation.to_matrix().to_4x4()@Matrix.Translation(-mat.translation)@mat
 bpy.context.view_layer.update()
def hand(side,point):
 upper='upperarm_'+side;lower='lowerarm_'+side;end='hand_'+side
 shoulder=target.pose.bones[upper].head.copy();elbow=target.pose.bones[lower].head.copy();wrist=target.pose.bones[end].head.copy()
 a=(elbow-shoulder).length;b=(wrist-elbow).length;d=point-shoulder
 length=min(a+b-.003,max(abs(a-b)+.003,d.length));axis=d.normalized()
 pole=Vector((.3 if side=='l' else -.3,-.08,-.1));pole=(pole-axis*pole.dot(axis)).normalized()
 along=(a*a+length*length-b*b)/(2*length);height=math.sqrt(max(0,a*a-along*along))
 aim(upper,lower,shoulder+axis*along+pole*height)
 aim(lower,end,shoulder+axis*length)
getup=[capture(f) for f in range(1200,1801,4)]
down=[capture(round(80+(720-80)*i/36)) for i in range(37)]
for i in range(31,37):down[i]=mix(down[i],getup[0],smooth((i-30)/6))
# Original rescue performance: kneel, extend both hands, support, then stand.
rescue=[]
stand=getup[-1];kneel=getup[85]
for i in range(151):
 seconds=i/30
 t=smooth(seconds/.65) if seconds<.65 else 1-smooth((seconds-2.5)/1.7)
 row=mix(stand,kneel,t);apply(row)
 root=target.pose.bones['root'];mat=root.matrix.copy();mat.translation.x=0;mat.translation.y=.07*t;root.matrix=mat;bpy.context.view_layer.update()
 reach=t*smooth(seconds/.4)
 for side,sign in [('l',1),('r',-1)]:
  wrist=target.pose.bones['hand_'+side].head.copy()
  point=Vector((sign*.12,-.34,.24+.34*smooth((seconds-2.4)/2)))
  hand(side,wrist.lerp(point,reach))
 rescue.append({p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in target.pose.bones})
from pipeline.common.paths import task_output_dir
out=task_output_dir('Hearthward','motion','TASK-095-survival','survival-motion-20261008');out.mkdir(parents=True,exist_ok=True)
def export(target,path):
 # Existing Hero/Brother bind poses inherit a 100x FBX root scale.
 # Preserve that convention; the centimetre-normalized archer exporter is incompatible.
 scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
 bpy.ops.object.select_all(action='DESELECT');target.select_set(True);bpy.context.view_layer.objects.active=target
 bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'ARMATURE'},
  add_leaf_bones=False,use_armature_deform_only=False,axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',
  bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,
  bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
report={'role':role,'source':'CMU 113_08, cgspeed BVH conversion','mapped_bones':mapping,'clips':[]}
for kind,rows in [('Down',down),('GetUp',getup),('Rescue',rescue)]:
 action=bpy.data.actions.new('A_'+role+'_'+kind);action.use_fake_user=True;target.animation_data.action=action
 previous={}
 for frame,row in enumerate(rows,1):
  for name,(loc,q,sc) in row.items():
   p=target.pose.bones[name];p.location=loc;p.rotation_quaternion=w.stabilize(q,previous.get(name));previous[name]=p.rotation_quaternion.copy();p.scale=sc
   p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
 scene.render.fps=30;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=len(rows);scene.frame_set(1)
 export(target,out/(action.name+'.fbx'))
 report['clips'].append({'name':action.name,'frames':len(rows),'seconds':(len(rows)-1)/30})
source.hide_set(True);target.animation_data.action=bpy.data.actions['A_'+role+'_GetUp'];scene.frame_end=151;scene.frame_set(1)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095'/(role+'-Survival.blend')))
(out/(role+'-author.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
print('SURVIVAL_AUTHORED',report)
