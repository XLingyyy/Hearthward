"""Verify geometry, UVs, weights and source conservation against R2 input."""
import bpy,json,hashlib,numpy as np
from pathlib import Path
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'corrections_20261001_r2'
SLUGS={'stag_a','hare','goat','pig','wolf','black_bear','ram','red_fox'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def meshdata(path):
 bpy.ops.wm.open_mainfile(filepath=str(path));result={}
 for o in bpy.context.scene.objects:
  if o.type!='MESH':continue
  m=o.data;raw=np.empty(len(m.vertices)*3,np.float32);m.vertices.foreach_get('co',raw)
  h=hashlib.sha256()
  for c,key,n,t in [(m.edges,'vertices',2,np.int32),(m.loops,'vertex_index',1,np.int32),(m.polygons,'loop_start',1,np.int32),(m.polygons,'loop_total',1,np.int32)]:
   a=np.empty(len(c)*n,t);c.foreach_get(key,a);h.update(a.tobytes())
  for u in m.uv_layers:
   a=np.empty(len(u.data)*2,np.float32);u.data.foreach_get('uv',a);h.update(u.name.encode());h.update(a.tobytes())
  weights=[[(o.vertex_groups[g.group].name,float(g.weight)) for g in v.groups] for v in m.vertices]
  result[o.name]={'vertices':raw.reshape((-1,3)),'topology_uv_sha256':h.hexdigest(),'weights':weights}
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
 return result,{'constraints':sum(len(p.constraints) for p in arm.pose.bones),'actions':set(a.name for a in bpy.data.actions),'bones':len(arm.data.bones)}
records=[]
for j in json.loads((ROOT/'jobs.json').read_text('utf-8')):
 if j['slug'] not in SLUGS:continue
 out=Path(j['output']);base=REV/'backup'/out.relative_to(ROOT.parent);old,_=meshdata(base/f'AS_{j["slug"]}.blend');new,scene=meshdata(out/f'AS_{j["slug"]}.blend');man=json.loads((out/'animation_manifest.json').read_text('utf-8'));head=man['correction_20261001_r2']['head_bind'];affected=set(head['head_bones']) if head else set();meshes=[]
 for name,a in old.items():
  b=new[name];changed=np.linalg.norm(a['vertices']-b['vertices'],axis=1)>1e-7;eligible=np.array([sum(w for n,w in gs if n in affected)>1e-7 for gs in a['weights']])
  meshes.append({'name':name,'vertex_count':len(changed),'changed_vertices':int(changed.sum()),'changes_outside_head_influence':int(np.sum(changed&~eligible)),'topology_uv_preserved':a['topology_uv_sha256']==b['topology_uv_sha256'],'weights_preserved':a['weights']==b['weights'],'max_vertex_change_cm':float(np.linalg.norm(a['vertices']-b['vertices'],axis=1).max()*100),'vertices_unchanged':not bool(changed.any()),'current_vertices_sha256':hashlib.sha256(b['vertices'].tobytes()).hexdigest(),'topology_uv_sha256':b['topology_uv_sha256']})
 record={'schema':'animal.integrity.r2','slug':j['slug'],'original_rigged_fbx_unchanged':sha(Path(j['source']))==j['source_sha256'],'geometry_and_uv_unchanged':all(x['vertices_unchanged'] and x['topology_uv_preserved'] for x in meshes),'local_head_geometry_corrected':bool(head),'topology_uv_weights_preserved':all(x['topology_uv_preserved'] and x['weights_preserved'] for x in meshes),'mesh_changes_outside_head':sum(x['changes_outside_head_influence'] for x in meshes),'meshes':meshes,'all_actions_present':all(c['name'] in scene['actions'] for c in man['clips']),'baked_constraints_removed':scene['constraints']==0,'actual_bone_count':scene['bones'],'blend_sha256':sha(out/f'AS_{j["slug"]}.blend'),'baseline_backup':str(base)}
 record['pass']=all(record[x] for x in ['original_rigged_fbx_unchanged','topology_uv_weights_preserved','all_actions_present','baked_constraints_removed']) and record['mesh_changes_outside_head']==0
 records.append(record);print('R2_INTEGRITY',j['slug'],record['pass'],flush=True)
 (out/'source_integrity_20261001_r2.json').write_text(json.dumps(record,ensure_ascii=False,indent=2),encoding='utf-8')
 # The established current integrity filename must no longer claim unchanged vertices for the three corrected heads.
 (out/'source_integrity_20261001.json').write_text(json.dumps({**record,'prior_revision_record':str(base/'source_integrity_20261001.json')},ensure_ascii=False,indent=2),encoding='utf-8')
(REV/'source_integrity.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
assert all(r['pass'] for r in records)

