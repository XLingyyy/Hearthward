"""Separate the generated settlement from its distant reconstructed mountains."""
from pathlib import Path
import json
import struct
import sys
import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent))
from pipeline.common.paths import task_output_dir, write_task_meta

source = trimesh.load(Path(__file__).with_name('stonehold-textured.glb'), force='scene')
mesh = source.to_geometry()
points = mesh.vertices[mesh.faces]
# Keep complete triangles only; no artificial surfaces across the crop boundary.
inside = ((points[:, :, 0] >= -45) & (points[:, :, 0] <= 40)
          & (points[:, :, 2] >= -90) & (points[:, :, 2] <= 45)
          & (points[:, :, 1] >= -15) & (points[:, :, 1] <= 80)).all(axis=1)
mesh.update_faces(inside)
mesh.remove_unreferenced_vertices()
mesh.visual.material.name = 'M_StoneholdVista'
scene = trimesh.Scene()
scene.add_geometry(mesh, node_name='SM_StoneholdVista', geom_name='SM_StoneholdVista')
raw = scene.export(file_type='glb')
json_size = struct.unpack_from('<I', raw, 12)[0]
doc = json.loads(raw[20:20+json_size])
doc['images'][0]['name'] = 'T_StoneholdColor'
data = json.dumps(doc, separators=(',', ':')).encode('utf-8')
data += b' ' * (-len(data) % 4)
tail = raw[20+json_size:]
raw = struct.pack('<4sII', b'glTF', 2, 20+len(data)+len(tail)) + struct.pack('<I4s', len(data), b'JSON') + data + tail
descriptor = dict(game_id='Hearthward', task_kind='3d_object', task_id='TASK-077-vista',
                  run_id='stonehold-20261006', artifact_key='model_path')
folder = task_output_dir(descriptor['game_id'], descriptor['task_kind'], descriptor['task_id'], run_id=descriptor['run_id'])
folder.mkdir(parents=True, exist_ok=True)
path = folder/'SM_StoneholdVista.glb'
path.write_bytes(raw)
write_task_meta(folder, {**descriptor, 'model_path': str(path)})
report = {**descriptor, 'file': str(path), 'triangles': len(mesh.faces), 'bounds_m': mesh.bounds.tolist(),
          'role': 'visual environment; gameplay collision remains authored separately'}
(ROOT/'docs/qa/TASK-077/prepared-mesh.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print(json.dumps(report))
