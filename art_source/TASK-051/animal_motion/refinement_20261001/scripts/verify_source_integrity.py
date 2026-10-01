"""Verify the editable deliverables against the archived original geometry."""
import bpy,json,hashlib
from pathlib import Path
import numpy as np
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果');REV=ROOT/'refinement_20261001'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def fingerprint(path):
 bpy.ops.wm.open_mainfile(filepath=str(path));result={}
 for obj in bpy.context.scene.objects:
  if obj.type!='MESH':continue
  m=obj.data;h=hashlib.sha256()
  for collection,key,n,dtype in [(m.vertices,'co',3,np.float32),(m.edges,'vertices',2,np.int32),(m.loops,'vertex_index',1,np.int32),(m.polygons,'loop_start',1,np.int32),(m.polygons,'loop_total',1,np.int32)]:
   data=np.empty(len(collection)*n,dtype=dtype);collection.foreach_get(key,data);h.update(data.tobytes())
  for uv in m.uv_layers:
   data=np.empty(len(uv.data)*2,dtype=np.float32);uv.data.foreach_get('uv',data);h.update(uv.name.encode());h.update(data.tobytes())
  result[obj.name]={'vertices':len(m.vertices),'edges':len(m.edges),'faces':len(m.polygons),'uv_layers':len(m.uv_layers),'geometry_uv_sha256':h.hexdigest()}
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
 return result,{'constraints':sum(len(p.constraints) for p in arm.pose.bones),'action_names':sorted(a.name for a in bpy.data.actions),'bone_count':len(arm.data.bones)}
records=[]
for job in json.loads((ROOT/'jobs.json').read_text('utf-8')):
 out=Path(job['output']);backup=REV/'backup'/out.relative_to(ROOT.parent)
 old,_=fingerprint(backup/f"AS_{job['slug']}.blend");new,scene=fingerprint(out/f"AS_{job['slug']}.blend")
 m=json.loads((out/'animation_manifest.json').read_text('utf-8'))
 record={'slug':job['slug'],'original_rigged_fbx_unchanged':sha(Path(job['source']))==job['source_sha256'],'geometry_and_uv_unchanged':old==new,'meshes':new,'all_actions_present':all(c['name'] in scene['action_names'] for c in m['clips']),'baked_constraints_removed':scene['constraints']==0,'actual_bone_count':scene['bone_count'],'blend_sha256':sha(out/f"AS_{job['slug']}.blend")}
 record['pass']=all(record[k] for k in ('original_rigged_fbx_unchanged','geometry_and_uv_unchanged','all_actions_present','baked_constraints_removed'));records.append(record);print('INTEGRITY',job['slug'],record['pass'],flush=True)
 (out/'source_integrity_20261001.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
(REV/'source_integrity.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
assert all(r['pass'] for r in records)
