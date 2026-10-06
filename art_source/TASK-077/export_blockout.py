"""Export the inspected runtime cubes as a small Chisel reference, in metres.

Run from GameFactory with its existing Python environment after the PIE verifier.
Furniture and the landscape are intentionally absent: this is reference geometry,
not a replacement for the UE level or an engine-ready generated asset.
"""
from pathlib import Path
import json,sys
import numpy as np
import trimesh

game=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(game.parent))
from pipeline.common.paths import task_output_dir

rows=json.loads((game/'.agent-local/qa/TASK-077/pie/blocks.json').read_text(encoding='utf-8'))
scene=trimesh.Scene()
for row in rows:
    mesh=trimesh.creation.box(extents=np.array(row['size_cm'])/100)
    mesh.apply_translation(np.array(row['center_cm'])/100)
    # UE Z-up centimetres -> glTF Y-up metres, retaining a right-handed layout.
    mesh.apply_transform(np.array([[1,0,0,0],[0,0,1,0],[0,-1,0,0],[0,0,0,1]]))
    wood=any(word in row['name'] for word in ('Roof','Ceiling','Beam','Floor'))
    mesh.visual.face_colors=[98,72,49,255] if wood else [125,129,126,255]
    scene.add_geometry(mesh,node_name=row['name'],geom_name=row['name'])
output=task_output_dir('hearthward','3d_scene','TASK-077',run_id='stonehold-blockout-20261005')
output.mkdir(parents=True,exist_ok=True)
data=scene.export(file_type='glb')
(output/'hometown-blockout.glb').write_bytes(data)
target=Path(__file__).parent/'hometown-blockout.glb'
target.write_bytes(data)
print(json.dumps({'file':str(target),'parts':len(rows),'bytes':len(data),'units':'metres','purpose':'Chisel layout reference'}))
