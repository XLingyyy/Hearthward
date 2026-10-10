"""Offline full source-rig skinning and clearance check. Requires numpy/scipy.
Run task104_extract_grip_rigs.py with Blender first. Does not execute UE.
"""
import json
from pathlib import Path
import numpy as np
from scipy.spatial.transform import Rotation
ROOT=Path(__file__).resolve().parents[2];out=ROOT/'.agent-local/qa/TASK-104/grip'
profiles=json.loads((ROOT/'art_source/TASK-104/grip/hand_pose_profiles.json').read_text())['profiles'];fit={'fingers':profiles['Hero']['target_fingers'],'palm_hand_local':profiles['Hero']['palm_hand_local']};sourcepose=json.loads((ROOT/'docs/qa/TASK-055/actual-hand-skin-full-inputs.json').read_text())
D=np.diag([1,-1,1]);HREF={}
def getrig(role):
 d=json.loads((out/(role+'-rig.json')).read_text());refs={}
 for name,b in d['bones'].items():
  b=np.array(b['matrix']);r=np.eye(4);r[:3,:3]=(D@b[:3,:3]@D).T*100;r[3,:3]=b[:3,3]*[100,-100,100];refs[name]=r
 return d,refs
hd,hr=getrig('Hero')
def localrot(refs,name,parent):
 loc=refs[name]@np.linalg.inv(refs[parent]);rot=loc[:3,:3];rot/=np.linalg.norm(rot,axis=1)[:,None];return Rotation.from_matrix(rot.T)
ref_errors=[]
for bone in sourcepose['all_used_influence_bone_skinning_inputs']:
    archived=np.linalg.inv(np.array(bone['inverse_reference_matrix_row_major']).reshape(4,4))
    source=hr[bone['name']]
    ref_errors.append({'bone':bone['name'],'translation_error_cm':float(np.linalg.norm(archived[3,:3]-source[3,:3])),
                       'rotation_matrix_max_error':float(np.max(abs(archived[:3,:3]/100-source[:3,:3]/100)))})
assert max(r['translation_error_cm'] for r in ref_errors)<.001
assert max(r['rotation_matrix_max_error'] for r in ref_errors)<.0001
(ROOT/'docs/qa/TASK-104/grip/reference-rig-check.json').write_text(json.dumps({'source':'TASK-095 Hero source-cache FBX','comparison':'Archived production inverse reference matrices','bones':ref_errors},indent=2)+'\n')
delta={n:(localrot(hr,n,hd['bones'][n]['parent']).inv()*Rotation.from_quat(q)) for n,q in fit['fingers'].items()}
palm0=(np.r_[(hr['thumb_02_r'][3,:3]+hr['middle_02_r'][3,:3])*.5,1]@np.linalg.inv(hr['hand_r']))[:3]
offset=np.array(fit['palm_hand_local'])-palm0
summaries=[]
for role in ('Hero','Brother'):
 d,refs=getrig(role);scale=100*(180/97.869893 if role=='Hero' else 160/97.863766)
 palm=(np.r_[(refs['thumb_02_r'][3,:3]+refs['middle_02_r'][3,:3])*.5,1]@np.linalg.inv(refs['hand_r']))[:3]
 palm=np.array(profiles[role]['palm_hand_local'])
 axiscs=refs['index_01_r'][3,:3]-refs['pinky_01_r'][3,:3];axiscs/=np.linalg.norm(axiscs)
 handR=refs['hand_r'][:3,:3]/100;axis=handR@axiscs;axis/=np.linalg.norm(axis)
 bx=np.cross(axis,[0,0,1]);bx/=np.linalg.norm(bx);basis=np.array([bx,np.cross(axis,bx)]).T
 comp={n:r@np.linalg.inv(refs['hand_r']) for n,r in refs.items()};comp['hand_r']=np.eye(4)
 quats={}
 for n in fit['fingers']:
  parent=d['bones'][n]['parent'];local=refs[n]@np.linalg.inv(refs[parent]);q=Rotation.from_quat(profiles[role]['target_fingers'][n]);local[:3,:3]=q.as_matrix().T;comp[n]=local@comp[parent];quats[n]=q.as_quat().tolist()
 m=d['meshes'][0];points=np.array(m['vertices'])*[100,-100,100];v=np.column_stack((points,np.ones(len(points))));sk=np.zeros((len(v),3));selected=[]
 for i,(p,weights) in enumerate(zip(v,m['weights'])):
  for n,w in weights:sk[i]+=(p@np.linalg.inv(refs[n])@comp[n])[:3]*w*scale
  handweight=sum(w for n,w in weights if n=='hand_r' or n in fit['fingers'])
  selected.append(handweight>.2 and all(n not in ('thigh_r','thigh_twist_01_r') for n,w in weights))
 faces=np.array([[f[0],f[k],f[k+1]] for f in m['faces'] for k in range(1,len(f)-1) if all(selected[i] for i in f)])
 tri=(sk[faces]-palm*scale)@basis
 edges=np.roll(tri,-1,axis=1)-tri;cross=lambda a,b:a[...,0]*b[...,1]-a[...,1]*b[...,0]
 areas=cross(edges,-tri);inside=(areas.min(1)>=0)|(areas.max(1)<=0)
 t=np.clip(np.sum(-tri*edges,axis=2)/np.maximum(np.sum(edges*edges,axis=2),1e-20),0,1)
 distances=np.linalg.norm(tri+t[:,:,None]*edges,axis=2).min(1);distances[inside]*=-1
 summary={'role':role,'selected_triangles':len(faces),'min_axis_clearance_cm':float(distances.min()),'triangles_inside_0_9cm_radius':int(sum(distances<.9)),'palm_hand_local':palm.tolist(),'axis_hand':axis.tolist(),'scale':scale}
 summaries.append(summary);print(summary,flush=True)
 (out/(role+'-full-fit.json')).write_text(json.dumps({**summary,'skin':sk.tolist(),'faces':faces.tolist(),'target_fingers':quats,'deltas':{n:r.as_quat().tolist() for n,r in delta.items()},'palm_reference':palm0.tolist(),'palm_offset':offset.tolist()}))
(ROOT/'docs/qa/TASK-104/grip/full-rig-fit-summary.json').write_text(json.dumps(summaries,indent=2)+'\n')
assert all(r['min_axis_clearance_cm']>.9 and r['triangles_inside_0_9cm_radius']==0 for r in summaries), 'A hand surface intersects the shaft core cylinder'
