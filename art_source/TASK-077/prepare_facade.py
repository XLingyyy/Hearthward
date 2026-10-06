"""Extract a limited courtyard stone facade; native masonry supplies its closed backing."""
from pathlib import Path
import json, struct, sys
import numpy as np
import trimesh
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir,write_task_meta
desc=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-077-facade',run_id='stonehold-20261006',artifact_key='model_path')
folder=task_output_dir(desc['game_id'],desc['task_kind'],desc['task_id'],run_id=desc['run_id']);folder.mkdir(parents=True,exist_ok=True)
mesh=trimesh.load(Path(__file__).with_name('courtyard-v2-textured.glb'),force='scene').to_geometry()
v=mesh.vertices
inside=(v[:,0]>=-7)&(v[:,0]<=12)&(v[:,1]>=0)&(v[:,1]<=18)&(v[:,2]>=-21)&(v[:,2]<=-10)
mesh.update_faces(inside[mesh.faces].all(axis=1));mesh.remove_unreferenced_vertices()
atlas=folder/'facade_base_color.png';mesh.visual.material.baseColorTexture.save(atlas)
scene=trimesh.Scene();scene.add_geometry(mesh,node_name='SM_StoneholdFacade',geom_name='SM_StoneholdFacade')
raw=scene.export(file_type='glb');n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n])
for item in doc['meshes']:
    for primitive in item['primitives']:primitive.pop('material',None)
for key in ('materials','textures','images','samplers'):doc.pop(key,None)
encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);tail=raw[20+n:]
path=folder/'SM_StoneholdFacade.glb';path.write_bytes(struct.pack('<4sII',b'glTF',2,20+len(encoded)+len(tail))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+tail)
write_task_meta(folder,{**desc,'model_path':str(path),'material_path':str(atlas)})
report={'triangles':len(mesh.faces),'bounds_m':mesh.bounds.tolist(),'source':'courtyard-v2-textured.glb','method':'Bounds crop only; preserve UV and topology. Unlit source material, dynamic Tint for time of day. Native closed masonry supplies backing and collision.'}
(ROOT/'docs/qa/TASK-077/facade-prepared.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(report)
