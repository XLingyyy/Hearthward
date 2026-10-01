"""Targeted measured-pose QA for the three user-reported defects."""
import bpy,json,sys,math,hashlib,numpy as np
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
sys.path.insert(0,str(REV/'scripts/reference_helpers'));sys.path.insert(0,str(REV/'scripts'))
from author import reset
from preview import setup
def points(meshes):
 result=[];dg=bpy.context.evaluated_depsgraph_get()
 for o in meshes:
  ev=o.evaluated_get(dg);m=ev.to_mesh();p=np.empty(len(m.vertices)*3,np.float32);m.vertices.foreach_get('co',p);result.append(p.reshape((-1,3)));ev.to_mesh_clear()
 return np.concatenate(result)
def angular(a,b):
 return math.degrees(2*math.acos(min(1.,abs(a.dot(b)))))
def cross_segments(A,B):
 """Strict interior Moller-Trumbore tests; no shared-edge/vertex contacts."""
 answer=np.zeros(len(A),bool)
 for first,second in [(A,B),(B,A)]:
  e1=second[:,1]-second[:,0];e2=second[:,2]-second[:,0]
  for i in range(3):
   origin=first[:,i];direction=first[:,(i+1)%3]-origin
   h=np.cross(direction,e2);det=np.sum(e1*h,axis=1);valid=np.abs(det)>1e-10;inv=np.divide(1.,det,out=np.zeros_like(det),where=valid)
   s=origin-second[:,0];u=inv*np.sum(s*h,axis=1);q=np.cross(s,e1);v=inv*np.sum(direction*q,axis=1);t=inv*np.sum(e2*q,axis=1)
   answer|=valid&(u>1e-5)&(v>1e-5)&(u+v<1-1e-5)&(t>1e-5)&(t<1-1e-5)
 return answer
def collision_partitions(meshes,rig):
 assert len(meshes)==1
 o=meshes[0];o.data.calc_loop_triangles();tri=np.array([list(t.vertices) for t in o.data.loop_triangles],int)
 weights=np.zeros((len(o.data.vertices),4),np.float32);lower=np.zeros_like(weights);keys=['FL','FR','BL','BR']
 for v in o.data.vertices:
  for g in v.groups:
   name=o.vertex_groups[g.group].name
   for i,k in enumerate(keys):
    if name in rig['chains'][k]:weights[v.index,i]+=g.weight
    if name in rig['chains'][k][1:]:lower[v.index,i]+=g.weight
 xyz=np.array([list(v.co) for v in o.data.vertices])
 xmin=min(rig['bone_heads'][rig['chains'][k][0]][0] for k in keys)
 xmax=max(rig['bone_heads'][rig['chains'][k][0]][0] for k in keys)
 body_v=(weights.sum(axis=1)<.30)&(xyz[:,0]>xmin-.08)&(xyz[:,0]<xmax+.08)&(xyz[:,2]>.23)
 body=tri[np.all(body_v[tri],axis=1)]
 limbs={k:tri[np.all(lower[:,i][tri]>.65,axis=1)] for i,k in enumerate(keys)}
 return body,limbs
def collisions(xyz,body,limbs):
 btree=BVHTree.FromPolygons(xyz.tolist(),body.tolist(),all_triangles=True);record={}
 for k,tris in limbs.items():
  ltree=BVHTree.FromPolygons(xyz.tolist(),tris.tolist(),all_triangles=True);pairs=btree.overlap(ltree)
  if pairs:
   a=np.array([body[i] for i,j in pairs]);b=np.array([tris[j] for i,j in pairs]);n=int(cross_segments(xyz[a],xyz[b]).sum())
  else:n=0
  record[k]=n
 return record
results={'schema':'animal.targeted.qa.r2','gaits':[],'heads':{},'goat_rest':{},'scope':'Measured native baked foot motion, neutral face axis, joint sides and non-adjacent lower-limb/trunk triangle crossings'}
jobs=json.loads((ROOT/'jobs.json').read_text('utf-8'));selected=['stag_a','hare','goat','pig','wolf','black_bear','ram','red_fox']
for job in jobs:
 slug=job['slug']
 if slug not in selected:continue
 out=Path(job['output']);baseline=REV/'backup'/out.relative_to(ROOT.parent)
 for phase,folder in [('before',baseline),('after',out)]:
  man=json.loads((folder/'animation_manifest.json').read_text('utf-8'));rig=man['rig'];bpy.ops.wm.open_mainfile(filepath=str(folder/f'AS_{slug}.blend'));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE');meshes=[o for o in scene.objects if o.type=='MESH']
  for c in man['clips']:
   if c['kind']!='run' and not (slug=='hare' and c['kind']=='hop'):continue
   arm.animation_data.action=bpy.data.actions[c['name']];paths={k:[] for k in rig['chains']}
   for f in range(1,c['frames']):
    scene.frame_set(f)
    for k,ns in rig['chains'].items():paths[k].append(list(arm.pose.bones[ns[3]].matrix.translation))
   measurements={}
   for side,pair in [('front',('FL','FR')),('hind',('BL','BR'))]:
    x,y=[np.array(paths[k])[:,0] for k in pair];x-=x.mean();y-=y.mean();norm=np.linalg.norm(x)*np.linalg.norm(y)
    corr=np.array([float(np.dot(x,np.roll(y,i))/norm) for i in range(len(x))]);lag=int(np.argmax(corr));lag=min(lag,len(x)-lag)
    measurements[side]={'zero_lag_correlation':float(corr[0]),'measured_lr_lag_cycles':lag/len(x),'left_right_excursion_rms_cm':float(np.sqrt(np.mean((x-y)**2))*100),'samples':len(x)}
   results['gaits'].append({'slug':slug,'phase':phase,'name':c['name'],'pairs':measurements,'pass':phase=='before' or all(.18<=v['measured_lr_lag_cycles']<=.33 for v in measurements.values())})
  if slug=='goat':
   body,limbs=collision_partitions(meshes,rig);records=[]
   for c in [x for x in man['clips'] if x['kind'] in ('lie','rest','up')]:
    arm.animation_data.action=bpy.data.actions[c['name']];max_deviation=0;link_error=0;wrong_side=0;wrong_frames=0
    for f in range(1,c['frames']+1):
     scene.frame_set(f)
     for k,ns in rig['chains'].items():
      hip=arm.pose.bones[ns[0]].matrix.translation;foot=arm.pose.bones[ns[3]].matrix.translation
      midline=sum(arm.pose.bones[rig['chains'][side][0]].matrix.translation.y for side in ('FL','FR','BL','BR'))/4
      for n in ns[1:3]:
       y=arm.pose.bones[n].matrix.translation.y;max_deviation=max(max_deviation,max(0,min(hip.y,foot.y)-y,y-max(hip.y,foot.y)))
       intrusion=(midline-y) if k.endswith('L') else (y-midline)
       wrong_side=max(wrong_side,intrusion);wrong_frames+=int(intrusion>.002)
      for a,b in zip(ns[:3],ns[1:4]):
       link_error=max(link_error,(arm.pose.bones[a].tail-arm.pose.bones[b].head).length)
    check_frames=sorted({1+round((c['frames']-1)*p) for p in [0,.25,.5,.75,1]});surface=[]
    for f in check_frames:
     scene.frame_set(f);surface.append({'frame':f,'lower_limb_trunk_crossings':collisions(points(meshes),body,limbs)})
    records.append({'name':c['name'],'max_joint_outside_own_limb_corridor_cm':max_deviation*100,'max_wrong_side_intrusion_cm':wrong_side*100,'wrong_side_joint_frames':wrong_frames,'joint_connection_error_cm':link_error*100,'sampled_self_intersections':surface})
   results['goat_rest'][phase]={'body_triangle_count':len(body),'limb_triangle_counts':{k:len(v) for k,v in limbs.items()},'clips':records}
  if phase=='after':
   # Source clip endpoints are compared in world space, including independent start/stop.
   lookup={c['suffix']:c for c in man['clips']};starts=[c for c in man['clips'] if c['kind']=='start'];stops=[c for c in man['clips'] if c['kind']=='stop'];runs=[c for c in man['clips'] if c['kind']=='run'];run=runs[-1] if slug=='pig' else runs[0]
   idle=man['clips'][0]
   pairs=[(starts[0],run),(run,stops[0]),(stops[0],idle),(idle,starts[0])]
   seams=[]
   for a,b in pairs:
    arm.animation_data.action=bpy.data.actions[a['name']];scene.frame_set(a['frames']);pa={p.name:(p.matrix.translation.copy(),p.matrix.to_quaternion()) for p in arm.pose.bones}
    arm.animation_data.action=bpy.data.actions[b['name']];scene.frame_set(1);pb={p.name:(p.matrix.translation.copy(),p.matrix.to_quaternion()) for p in arm.pose.bones}
    seams.append({'from':a['name'],'to':b['name'],'position_cm':max((pa[n][0]-pb[n][0]).length*100 for n in pa),'rotation_deg':max(angular(pa[n][1],pb[n][1]) for n in pa)})
   results.setdefault('start_stop_seams',{})[slug]=seams
   cam,_=setup(arm,rig['axis']['target_length_m']);scene.render.resolution_x=600;scene.render.resolution_y=500
   if slug in ('stag_a','ram','red_fox'):
    arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update();target=Vector((.5,0,1.25 if slug!='red_fox' else .96))
    cam.location=target+Vector((2,0,.05));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(REV/'inspection'/f'{slug}_corrected_head_front.png');bpy.ops.render.render(write_still=True)
   if slug=='goat':
    arm.animation_data.action=bpy.data.actions['AN_goat_Rest'];scene.frame_set(1)
    for name,loc in [('side',(0,-3,.74)),('front',(3,0,.72)),('top',(0,0,3))]:
     target=Vector((0,0,.35));cam.location=loc;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(REV/'inspection'/f'goat_corrected_rest_{name}.png');bpy.ops.render.render(write_still=True)
 print('TARGETED',slug,flush=True)
(REV/'targeted_qa.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')

