"""Correct neutral head binding, staggered fast steps, and planar goat rest.
Repeatable from the immutable R2 backup; no source FBX or game edits.
"""
import bpy,json,sys,math,hashlib,numpy as np
from pathlib import Path
from mathutils import Vector,Quaternion,Matrix
from mathutils.kdtree import KDTree
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts/reference_helpers'));sys.path.insert(0,str(REV/'scripts'))
from author import reset,curves,export_fbx,write_rig_report,foot_x,smooth,evaluated_min_z
from refinement_helpers_r1 import snapshot,apply,move,rotate,blend,build_ik,bake_frame
from pose_tools import solve_three
def save(p,v):p.write_text(json.dumps(v,ensure_ascii=False,indent=2),encoding='utf-8')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def pts(meshes):
 dg=bpy.context.evaluated_depsgraph_get();result=[]
 for o in meshes:
  ev=o.evaluated_get(dg);m=ev.to_mesh();v=np.empty(len(m.vertices)*3,np.float32);m.vertices.foreach_get('co',v)
  result.append(v.reshape((-1,3)));ev.to_mesh_clear()
 return np.concatenate(result)
def sample(values,t):
 f=min(len(values)-1,max(0,t*(len(values)-1)));i=int(f)
 return blend(values[i],values[min(i+1,len(values)-1)],f-i)
def rebind_head(arm,meshes,rig,slug,original):
 fit=json.loads((REV/'head_fit.json').read_text('utf-8'))[slug];driver=fit['driver'];yaw=fit['correction_yaw_deg'];region=fit['region']
 old={b.name:b.matrix_local.copy() for b in arm.data.bones};affected={driver}|{b.name for b in arm.data.bones[driver].children_recursive}
 arm.animation_data.action=None;reset(arm);rotate(arm,driver,(0,0,1),yaw);bpy.context.view_layer.update()
 xyz=pts(meshes);cloud=xyz[(xyz[:,0]>region['x_min'])&(xyz[:,2]>region['z_min'])&(xyz[:,2]<region['z_max'])]
 tree=KDTree(len(cloud))
 for i,p in enumerate(cloud):tree.insert(p,i)
 tree.balance();test=cloud[np.random.default_rng(8).choice(len(cloud),min(800,len(cloud)),replace=False)]
 center=cloud.mean(axis=0);choices=[]
 for y in np.arange(center[1]-.05,center[1]+.0501,.002):
  mirror=test.copy();mirror[:,1]=2*y-mirror[:,1];d=np.sort([tree.find(p)[2]**2 for p in mirror])
  choices.append((float(np.mean(d[:int(.9*len(d))])),float(y)))
 face_y=min(choices)[1];body_y=sum(arm.data.bones[rig['chains'][k][0]].head_local.y for k in ('FL','FR'))/2
 shift=body_y-face_y;move(arm,driver,(0,shift,0));bpy.context.view_layer.update()
 matrices={n:arm.pose.bones[n].matrix.copy() for n in affected};dg=bpy.context.evaluated_depsgraph_get();changed=0;other_error=0
 for o in meshes:
  ev=o.evaluated_get(dg);m=ev.to_mesh();coordinates=[v.co.copy() for v in m.vertices];ev.to_mesh_clear()
  for v,pos in zip(o.data.vertices,coordinates):
   if sum(g.weight for g in v.groups if o.vertex_groups[g.group].name in affected)>1e-7:
    if (v.co-pos).length>1e-7:changed+=1
    v.co=pos
   else:other_error=max(other_error,(v.co-pos).length)
 bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 for n in affected:arm.data.edit_bones[n].matrix=matrices[n]
 bpy.ops.object.mode_set(mode='OBJECT');reset(arm);bpy.context.view_layer.update()
 conversions={n:arm.data.bones[n].matrix_local.to_quaternion().inverted() @ old[n].to_quaternion() for n in affected}
 for poses in original.values():
  for p in poses:
   for n,c in conversions.items():
    loc,q=p[n];p[n]=(c@loc,c@q@c.inverted())
 # This is a head rebind, not global scaling. Record the new measured muzzle-to-tail extent.
 body_length=rig['axis']['target_length_m'];xyz=pts(meshes)
 rig['axis']['body_reference_length_m']=body_length;rig['axis']['target_length_m']=float(np.ptp(xyz[:,0]))
 rig['axis']['head_rebind_length_note']='Measured +X extent after local head correction; body and limb scale unchanged'
 rig['repairs'].append(f'2026-10-01 R2: neutral {driver} yaw {yaw:+.1f} deg, head-centre Y shift {shift*100:+.2f} cm; locally rebaked mesh and rest matrices; global-axis motion retargeted')
 return {'driver':driver,'yaw_deg':yaw,'head_centre_shift_cm':shift*100,'head_bones':sorted(affected),'changed_head_vertices':changed,'unaffected_vertex_pose_error_m':other_error,'old_length_m':body_length,'new_length_m':rig['axis']['target_length_m'],'topology_uv_weights':'preserved'}
def goat_rest(arm,rig,standing,heads,a,t):
 apply(arm,standing);move(arm,rig['body'],(0,0,-.42*a))
 for n in rig['spine'][:2]:rotate(arm,n,(0,1,0),.12*math.sin(2*math.pi*t)*a)
 bpy.context.view_layer.update()
 for key,ns in rig['chains'].items():
  front=key.startswith('F');hip=arm.pose.bones[ns[0]].matrix.translation.copy();foot=heads[ns[3]].copy();foot.x+=(.20 if front else -.07)*a
  original_dir=(heads[ns[1]]-heads[ns[0]]).normalized()
  angle=math.radians(45 if front else 55);fold=Vector(((-1 if front else 1)*math.sin(angle),0,-math.cos(angle)))
  upper=original_dir.lerp(fold,a).normalized()
  lower=heads[ns[3]]-heads[ns[1]];bend=heads[ns[2]]-heads[ns[1]];initial=lower.cross(bend).normalized()
  # normal x lower points towards the original elbow/knee side at a=0.
  desired=Vector((0,1 if front else -1,0));normal=initial.lerp(desired,a)
  if normal.length<.05:normal=desired
  original_y=[heads[n].y for n in ns[:4]]
  lower=min(original_y)*(1-a)+min(hip.y,foot.y)*a-.002
  upper_bound=max(original_y)*(1-a)+max(hip.y,foot.y)*a+.002
  solve_three(arm,ns,hip,foot,upper,normal.normalized(),heads,(lower,upper_bound))
def author(job):
 slug=job['slug'];out=Path(job['output']);base=REV/'backup'/out.relative_to(ROOT.parent);man=json.loads((base/'animation_manifest.json').read_text('utf-8'));rig=man['rig']
 bpy.ops.wm.open_mainfile(filepath=str(base/f'AS_{slug}.blend'));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE');meshes=[o for o in scene.objects if o.type=='MESH' and not o.name.startswith('BindPoseProxy')]
 original={}
 for c in man['clips']:
  arm.animation_data.action=bpy.data.actions[c['name']];poses=[]
  for f in range(1,c['frames']+1):scene.frame_set(f);poses.append(snapshot(arm))
  original[c['suffix']]=poses
 arm.animation_data.action=None;head_info=None
 if slug in ('stag_a','ram','red_fox'):head_info=rebind_head(arm,meshes,rig,slug,original)
 standing=original[man['clips'][0]['suffix']][0];apply(arm,standing);bpy.context.view_layer.update();heads={p.name:p.matrix.translation.copy() for p in arm.pose.bones}
 length=rig['axis'].get('body_reference_length_m',rig['axis']['target_length_m'])
 targets=build_ik(arm,rig,standing);leg_names={b.name for ns in rig['chains'].values() for b in [arm.data.bones[ns[0]],*arm.data.bones[ns[0]].children_recursive]}
 rest_final=None
 if slug=='goat':
  arm.animation_data.action=None;goat_rest(arm,rig,standing,heads,1,0);bpy.context.view_layer.update();rest_final=snapshot(arm)
 run_clips=[c for c in man['clips'] if c['kind']=='run'];main_run=run_clips[-1] if slug=='pig' else run_clips[0];changed=[];metrics={}
 for c in man['clips']:
  fast=c['kind'] in ('run','start','stop') or (slug=='hare' and c['kind']=='hop');rest=slug=='goat' and c['kind'] in ('lie','rest','up')
  if not (head_info or fast or rest):continue
  oldaction=bpy.data.actions[c['name']];bpy.data.actions.remove(oldaction);action=bpy.data.actions.new(c['name']);action.use_fake_user=True
  for k in ('fps','loop','description'):action[k]=c[k]
  arm.animation_data.action=action;frames=[];notes=[]
  if fast:
   template=c if c['kind'] in ('run','hop') else main_run;duty=.30 if c['kind']=='hop' else .24 if slug=='black_bear' else .20 if slug in ('pig','hare') else .18
   phases={'BL':0.,'BR':.25,'FL':.50,'FR':.75}
   if slug=='red_fox':phases={'BR':0.,'BL':.25,'FR':.50,'FL':.75}
   cycles=template.get('gait_cycles',1);stride=template['reference_speed_cm_s']/100*template['duration']*template['contact_duty']/cycles
   lift=(.065 if c['kind']=='hop' else .095 if slug=='hare' else .11 if slug=='black_bear' else .08)*length/(.65 if slug=='hare' else 1.6)
   c.update(contact_duty=duty,contact_offsets=phases,gait_cycles=cycles,reference_speed_cm_s=round(stride*cycles/(c['duration']*duty)*100,3))
   c['gait_revision']={'left_right_separation_cycles':.25,'stride_m':stride,'swing_lift_m':lift,'step_order':'BR → BL → FR → FL' if slug!='red_fox' else 'BL → BR → FL → FR','reference_speed_usage':'constant at loop rate; start/stop use actor acceleration and amplitude envelope','independent_targets':True}
   notes.append('Four separate foot trajectories with quarter-cycle left/right timing; no paired-leg lock')
  if head_info:notes.append('Neutral forward head bind and all motion channels retargeted to corrected rest orientation')
  if rest:notes.append('Explicit sagittal terminal fold and continuous local quaternion interpolation; fixed transverse twist frame; no unconstrained rest IK or cross-body knees')
  previous={}
  for f in range(1,c['frames']+1):
   t=(f-1)/(c['frames']-1);scene.frame_set(f)
   for _,con,_,_,_ in targets.values():con.mute=True
   active=False
   if fast:
    amplitude=smooth(t) if c['kind']=='start' else 1-smooth(t) if c['kind']=='stop' else 1
    source=sample(original[template['suffix']],t)
    for n in leg_names:source[n]=standing[n]
    apply(arm,blend(standing,source,amplitude));bpy.context.view_layer.update()
    for key,(ctl,con,eff,point,q) in targets.items():
     phase=(t*cycles+phases[key])%1;x,z,contact=foot_x(phase,duty,stride)
     ctl.location=point+Vector((x*amplitude,0,z*lift*amplitude));con.mute=False;con.influence=1
    active=True
   elif rest:
    a=smooth(t) if c['kind']=='lie' else 1-smooth(t) if c['kind']=='up' else 1
    apply(arm,blend(standing,rest_final,a))
    for n in rig['spine'][:2]:rotate(arm,n,(0,1,0),.12*math.sin(2*math.pi*t)*a)
   else:apply(arm,original[c['suffix']][f-1])
   if active:
    bpy.context.view_layer.update()
    for ctl,con,eff,point,q in targets.values():
     p=arm.pose.bones[eff];mat=q.to_matrix().to_4x4();mat.translation=p.matrix.translation;p.matrix=mat
    bpy.context.view_layer.update()
   if rest:
    # Feet are allowed to tuck during the transition; preserve contact height
    # after baking the smooth connected-joint rotation path.
    low=evaluated_min_z(meshes)
    if low<0:move(arm,rig['body'],(0,0,-low+.0003))
   if head_info and c['kind'] in ('collapse','corpse'):
    low=evaluated_min_z(meshes)
    if low<0:move(arm,rig['body'],(0,0,-low+.0003))
   bake_frame(arm,action,f,targets if active else None)
   for p in arm.pose.bones:
    q=p.rotation_quaternion
    if p.name in previous and previous[p.name].dot(q)<0:q.negate();p.keyframe_insert('rotation_quaternion',frame=f,group=p.name)
    previous[p.name]=q.copy()
   frames.append(snapshot(arm))
  for fc in curves(action):
   for k in fc.keyframe_points:k.interpolation='LINEAR'
  apply(arm,frames[0]);arm.animation_data.action=action;scene.render.fps=c['fps'];scene.frame_start=1;scene.frame_end=c['frames'];scene.frame_set(1)
  export_fbx(arm,meshes,out/'clips'/f'{c["name"]}.fbx',False,True);c['sha256']=sha(Path(c['file']));c['skeleton_signature']='pending_current_bind'
  c['correction_20261001_r2']={'changes':notes,'baseline_fbx_sha256':next(x['sha256'] for x in json.loads((base/'animation_manifest.json').read_text('utf-8'))['clips'] if x['name']==c['name']),'qa':'PENDING'}
  c['qa']={'status':'PENDING_R2_REVALIDATION'};c['delivery_review'].update(numeric_qa='PENDING',native_source_verified=False,fbx_source_verified=False)
  changed.append(c['name']);print('CORRECTED',slug,c['suffix'],flush=True)
  arm.animation_data.action=None
 # Remove author-only IK objects and constraints before saving.
 for ctl,con,eff,point,q in targets.values():
  for p in arm.pose.bones:
   if con in list(p.constraints):p.constraints.remove(con)
  bpy.data.objects.remove(ctl,do_unlink=True)
 reset(arm);bpy.context.view_layer.update();write_rig_report(arm,meshes,rig,out)
 for c in man['clips']:c['skeleton_signature']=rig['skeleton_signature']
 if head_info:export_fbx(arm,meshes,out/f'SK_{slug}.fbx',True,False)
 man['rig']=rig;man['ue_import']='PENDING_R2';man['visual_qa']='PENDING_R2'
 man['correction_20261001_r2']={'baseline_backup':str(base),'changed_actions':changed,'head_bind':head_info,'rest_limb_planes':'explicit anatomical front/hind fold; 0.42 m trunk descent' if slug=='goat' else None,'cloud_credits_used':0,'baked_constraints_removed':True,'checks':'PENDING'}
 if head_info:man['reference_pose_calibration']['current_mesh_note']='R2 local head vertices and rest corrected; earlier calibration fields describe historical R0 only'
 save(out/'animation_manifest.json',man);save(out/'correction_20261001_r2.json',man['correction_20261001_r2'])
 save(out/'rig_audit.json',{'schema':'animal.rig.audit.r2','rig':rig,'skin_qa':rig['skin_qa'],'head_bind':head_info,'source':'current native bind'})
 arm.animation_data.action=bpy.data.actions[man['clips'][0]['name']];scene.render.fps=man['clips'][0]['fps'];scene.frame_start=1;scene.frame_end=man['clips'][0]['frames'];scene.frame_set(1)
 bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(out/f'AS_{slug}.blend'))
 print('R2_AUTHORED',slug,len(changed),flush=True)
def main():
 jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'));args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
 for j in jobs:
  if j['slug'] in args:author(j)
if __name__=='__main__':main()

