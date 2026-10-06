"""Preserve the textured export's geometry and UVs for an independent UE comparison."""
from pathlib import Path
import json, struct, sys
import trimesh

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir,write_task_meta

desc=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-077-courtyard-v2',run_id='stonehold-20261006',artifact_key='model_path')
folder=task_output_dir(desc['game_id'],desc['task_kind'],desc['task_id'],run_id=desc['run_id'])
mesh=trimesh.load(Path(__file__).with_name('courtyard-v2-textured.glb'),force='scene').to_geometry()
atlas=folder/'courtyard_base_color.png'
mesh.visual.material.baseColorTexture.save(atlas)
scene=trimesh.Scene();scene.add_geometry(mesh,node_name='SM_StoneholdCourtyard',geom_name='SM_StoneholdCourtyard')
raw=scene.export(file_type='glb')
n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n])
# The public binding API imports the atlas; retain UVs without duplicate imported materials.
for item in doc['meshes']:
    for primitive in item['primitives']:primitive.pop('material',None)
for key in ('materials','textures','images','samplers'):doc.pop(key,None)
encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4)
tail=raw[20+n:]
path=folder/'SM_StoneholdCourtyard.glb'
path.write_bytes(struct.pack('<4sII',b'glTF',2,20+len(encoded)+len(tail))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+tail)
write_task_meta(folder,{**desc,'model_path':str(path),'material_path':str(atlas)})
report={'variant':'textured original geometry, baked node transform; no clustering or crop','triangles':len(mesh.faces),'bounds_m':mesh.bounds.tolist(),'file':str(path),'atlas':str(atlas),'source_material':'KHR_materials_unlit; separate atlas binding and unlit QA view required for this diagnostic derivative'}
(ROOT/'docs/qa/TASK-077/courtyard-v2-textured-prepared.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report),flush=True)
