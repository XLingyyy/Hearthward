"""Reduce the dense vertex-colored export using 5 cm spatial clusters."""
from pathlib import Path
import json, struct, sys
import numpy as np
import trimesh

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT.parent))
from pipeline.common.paths import task_output_dir,write_task_meta

raw=Path(__file__).with_name('courtyard-v2-vertex-colored.glb').read_bytes()
n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n]);binary=memoryview(raw)[28+n:]
def accessor(index):
    a=doc['accessors'][index];view=doc['bufferViews'][a['bufferView']]
    dtype={5126:'<f4',5125:'<u4',5121:'u1'}[a['componentType']]
    width={'SCALAR':1,'VEC3':3,'VEC4':4}[a['type']]
    return np.frombuffer(binary,dtype=dtype,count=a['count']*width,offset=view.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)
primitive=doc['meshes'][0]['primitives'][0]
v=accessor(primitive['attributes']['POSITION']).copy()
t=np.array(doc['nodes'][0]['matrix'],dtype=np.float32).reshape(4,4,order='F')
v=v@t[:3,:3].T+t[:3,3]
color=accessor(primitive['attributes']['COLOR_0'])
faces=accessor(primitive['indices']).reshape(-1,3)
cell=.05
grid=np.floor((v-v.min(axis=0))/cell).astype(np.int64)
extent=grid.max(axis=0)+1
codes=(grid[:,0]*extent[1]+grid[:,1])*extent[2]+grid[:,2]
del grid
_,inverse,counts=np.unique(codes,return_inverse=True,return_counts=True)
del codes
size=len(counts)
positions=np.stack([np.bincount(inverse,weights=v[:,i],minlength=size)/counts for i in range(3)],axis=1).astype(np.float32)
colors=np.stack([np.bincount(inverse,weights=color[:,i],minlength=size)/counts for i in range(4)],axis=1).round().astype(np.uint8)
inverse=inverse.astype(np.uint32)
reduced=[]
for start in range(0,len(faces),1000000):
    f=inverse[faces[start:start+1000000]]
    keep=(f[:,0]!=f[:,1])&(f[:,1]!=f[:,2])&(f[:,2]!=f[:,0])
    reduced.append(f[keep])
f=np.concatenate(reduced);del reduced
_,unique=np.unique(np.sort(f,axis=1),axis=0,return_index=True)
f=f[np.sort(unique)]
sky=(colors[:,1].astype(int)>colors[:,0].astype(int)+1)&(colors[:,1]>130)&(positions[:,1]>8)
keep=(positions[f,1]<=30).all(axis=1)&(positions[f,1]>=-3).all(axis=1)&~sky[f].all(axis=1)
removed=int((~keep).sum());f=f[keep]
mesh=trimesh.Trimesh(vertices=positions,faces=f,vertex_colors=colors,process=False)
mesh.remove_unreferenced_vertices()
scene=trimesh.Scene();scene.add_geometry(mesh,node_name='SM_StoneholdCourtyard',geom_name='SM_StoneholdCourtyard')
desc=dict(game_id='Hearthward',task_kind='3d_object',task_id='TASK-077-courtyard-v2',run_id='stonehold-20261006',artifact_key='model_path')
folder=task_output_dir(desc['game_id'],desc['task_kind'],desc['task_id'],run_id=desc['run_id']);folder.mkdir(parents=True,exist_ok=True)
path=folder/'SM_StoneholdCourtyard.glb';path.write_bytes(scene.export(file_type='glb'))
write_task_meta(folder,{**desc,'model_path':str(path),'material_path':str(path)})
report={**desc,'file':str(path),'source_triangles':len(faces),'triangles':len(mesh.faces),'vertices':len(mesh.vertices),'cell_m':cell,'sky_and_background_faces_removed':removed,'bounds_m':mesh.bounds.tolist(),'method':'Spatial clustering; averaged positions and vertex colors, degenerate and duplicate faces removed. Removed background above 30m, below -3m and pale green sky samples above 8m. No generated closure surfaces.'}
(ROOT/'docs/qa/TASK-077/courtyard-v2-prepared.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report),flush=True)
