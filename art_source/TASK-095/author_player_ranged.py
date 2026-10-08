"""Original player bow and crossbow poses on the existing Hero rig."""
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
def hand(side,grip,orientation=None,pole=None):
 weapon=orientation.copy() if orientation is not None else axes(Vector((0,-1,0)));weapon.translation=Vector(grip)
 desired=weapon@GRIPS[side].inverted();point=desired.translation
 upper='upperarm_'+side;lower='lowerarm_'+side;end='hand_'+side
 shoulder=rig.pose.bones[upper].head.copy();elbow=rig.pose.bones[lower].head.copy();wrist=rig.pose.bones[end].head.copy()
 a=(elbow-shoulder).length;b=(wrist-elbow).length;d=point-shoulder
 distance=min(a+b-.001,max(abs(a-b)+.001,d.length));axis=d.normalized()
 pole=Vector((.4 if side=='l' else -.4,.05,-.3)) if pole is None else pole;pole=(pole-axis*pole.dot(axis)).normalized()
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

# Match the left-hand longbow attachment in HearthwardCharacter.
bow_rest=Matrix.Rotation(-math.pi/2,4,'Z')
bow_rest.translation=(rig.data.bones['thumb_02_l'].head_local+rig.data.bones['middle_02_l'].head_local)*.5
BOW_GRIP=REST['hand_l'].inverted()@bow_rest
SOURCE_SCALE=.9786989/1.8
out=task_output_dir('Hearthward','motion','TASK-095-player-ranged','ranged-20261008');out.mkdir(parents=True,exist_ok=True)
report=[]
for label,seconds in [('BowDraw',1),('BowRelease',.35),('CrossbowAim',.5),('CrossbowReload',1.8)]:
 rows=[];errors=[];count=round(seconds*60)
 for i in range(count+1):
  t=i/count;reset();draw=smooth(t) if label=='BowDraw' else 1-smooth(t)
  turn=-65*draw if label.startswith('Bow') else -25
  spine=rig.pose.bones['spine_01'];pivot=spine.head.copy()
  spine.matrix=Matrix.Translation(pivot)@Matrix.Rotation(math.radians(turn),4,'Z')@Matrix.Translation(-pivot)@spine.matrix
  bpy.context.view_layer.update();head=rig.pose.bones['head'];pivot=head.head.copy()
  head.matrix=Matrix.Translation(pivot)@Matrix.Rotation(math.radians(-turn),4,'Z')@Matrix.Translation(-pivot)@head.matrix
  bpy.context.view_layer.update()
  if label.startswith('Bow'):
   draw=smooth(t) if label=='BowDraw' else 1-smooth(t)
   left=Vector((-.03,-.20-.105*draw,.81));right=left+Vector((0,(.15+.35*draw)*SOURCE_SCALE,.018))
   if label=='BowRelease':right+=Vector((-.025,.025,0))*math.sin(math.pi*t)
   original=GRIPS['l'];GRIPS['l']=BOW_GRIP
   errors.append([hand('l',left,Matrix.Rotation(-math.pi/2,4,'Z')),hand('r',right,Matrix.Identity(4),pole=Vector((-.3,.2,-.04)))])
   GRIPS['l']=original
   for side,grip in [('l',left),('r',right)]:
    for finger in ['index','middle','ring','pinky']:
     for joint in ['02','03']:
      p=rig.pose.bones[finger+'_'+joint+'_'+side];point=p.head.copy();closest=Vector((grip.x,grip.y,point.z));radial=point-closest
      target=closest+radial.normalized()*.007
      q=(p.tail-point).normalized().rotation_difference((target-point).normalized())
      if q.angle>math.radians(65):q=q.slerp(type(q)(),1-math.radians(65)/q.angle)
      p.matrix=Matrix.Translation(point)@q.to_matrix().to_4x4()@Matrix.Translation(-point)@p.matrix;bpy.context.view_layer.update()
  else:
   right=Vector((-.06,-.10,.71));left=Vector((-.06,-.23,.73))
   if label=='CrossbowReload':
    lower=math.sin(math.pi*t)**2
    right.z-=.11*lower;left.z-=.11*lower
    left.y+=.10*math.sin(math.pi*t)**2
   else:
    recoil=.013*math.sin(math.pi*t)*math.exp(-4*t)
    right.y+=recoil;left.y+=recoil
   orientation=axes(Vector((0,-1,0)))
   errors.append([hand('r',right,orientation),hand('l',left)])
   close_fingers('r',right);close_fingers('l',left)
  rows.append({p.name:(p.location.copy(),p.rotation_quaternion.copy(),p.scale.copy()) for p in rig.pose.bones})
 max_error=max(max(e) for e in errors)
 assert max_error<.003,(label,max_error,errors[0],errors[-1])
 action=bpy.data.actions.new('A_Hero_'+label);action.use_fake_user=True;rig.animation_data.action=action;previous={}
 for frame,row in enumerate(rows,1):
  for n,(loc,q,scale) in row.items():
   if n in previous and q.dot(previous[n])<0:q.negate()
   previous[n]=q.copy();p=rig.pose.bones[n];p.location=loc;p.rotation_quaternion=q;p.scale=scale
   p.keyframe_insert('location',frame=frame);p.keyframe_insert('rotation_quaternion',frame=frame);p.keyframe_insert('scale',frame=frame)
 scene.render.fps=60;scene.render.fps_base=1;scene.frame_start=1;scene.frame_end=count+1;scene.frame_set(1)
 scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
 bpy.ops.object.select_all(action='DESELECT');rig.select_set(True);bpy.context.view_layer.objects.active=rig
 bpy.ops.export_scene.fbx(filepath=str(out/(action.name+'.fbx')),use_selection=True,object_types={'ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='X',axis_up='Z',apply_scale_options='FBX_SCALE_NONE',bake_anim=True,bake_anim_use_all_bones=True,bake_anim_use_nla_strips=False,bake_anim_use_all_actions=False,bake_anim_force_startend_keying=True,bake_anim_step=1,bake_anim_simplify_factor=0)
 report.append({'clip':action.name,'seconds':seconds,'maximum_grip_error_source_m':max_error})
rig.animation_data.action=bpy.data.actions['A_Hero_BowDraw'];scene.frame_start=1;scene.frame_end=61;scene.frame_set(61)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'art_source/TASK-095/Hero-Ranged.blend'))
(out/'author.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print('RANGED_AUTHORED',json.dumps(report))
