"""Check actual deformed sole clusters instead of preview markers."""
import bpy,json,hashlib,numpy as np
from pathlib import Path
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2';records=[]
for j in json.loads((ROOT/'jobs.json').read_text('utf-8')):
 if j['slug'] not in {'stag_a','hare','goat','pig','wolf','black_bear','ram','red_fox'}:continue
 slug=j['slug'];out=Path(j['output']);m=json.loads((out/'animation_manifest.json').read_text('utf-8'));rig=m['rig'];L=rig['axis'].get('body_reference_length_m',rig['axis']['target_length_m'])
 bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{slug}.blend'));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');mesh=next(o for o in bpy.context.scene.objects if o.type=='MESH');xyz=np.array([list(v.co) for v in mesh.data.vertices]);masks={}
 for k,ns in rig['chains'].items():
  foot=np.array(rig['restfeet'][k]);weights=np.array([sum(g.weight for g in v.groups if mesh.vertex_groups[g.group].name in ns[2:]) for v in mesh.data.vertices])
  mask=(xyz[:,2]<L*.028)&(np.abs(xyz[:,0]-foot[0])<L*.08)&(np.abs(xyz[:,1]-foot[1])<L*.06)&(weights>.65)
  assert mask.sum()>5,(slug,k,int(mask.sum()))
  masks[k]=mask
 for c in [c for c in m['clips'] if c['kind']=='run']:
  arm.animation_data.action=bpy.data.actions[c['name']];paths={k:[] for k in masks}
  for f in range(1,c['frames']):
   bpy.context.scene.frame_set(f);ev=mesh.evaluated_get(bpy.context.evaluated_depsgraph_get());data=ev.to_mesh();a=np.empty(len(data.vertices)*3,np.float32);data.vertices.foreach_get('co',a);a=a.reshape((-1,3))
   for k,mask in masks.items():paths[k].append(a[mask].mean(axis=0).tolist())
   ev.to_mesh_clear()
  pairs={}
  for key,(left,right) in [('front',('FL','FR')),('hind',('BL','BR'))]:
   x,y=[np.array(paths[k])[:,0] for k in (left,right)];x-=x.mean();y-=y.mean();norm=np.linalg.norm(x)*np.linalg.norm(y);corr=[float(np.dot(x,np.roll(y,i))/norm) for i in range(len(x))];lag=int(np.argmax(corr));lag=min(lag,len(x)-lag)/len(x)
   pairs[key]={'measured_skin_lr_lag_cycles':lag,'zero_lag_correlation':corr[0],'excursion_rms_difference_cm':float(np.sqrt(np.mean((x-y)**2))*100)}
  record={'slug':slug,'name':c['name'],'source_blend_sha256':hashlib.sha256((out/f'AS_{slug}.blend').read_bytes()).hexdigest(),'sole_vertex_counts':{k:int(v.sum()) for k,v in masks.items()},'pairs':pairs,'pass':all(.18<=p['measured_skin_lr_lag_cycles']<=.33 for p in pairs.values())};records.append(record);print('SKIN_PHASE',slug,c['suffix'],record['pass'],pairs,flush=True)
(REV/'skin_sole_phase_qa.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
assert len(records)==9 and all(r['pass'] for r in records)

