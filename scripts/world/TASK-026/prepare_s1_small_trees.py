"""Deterministic small-tree clusters beside the existing S1 walk loop."""
from pathlib import Path
import json
import math
import numpy as np
from scipy.spatial import cKDTree
from prepare_dressing import h,slope,wet,sample
from rework_contract import stable_seed

root=Path(__file__).resolve().parents[3]
source=root/'art_source/TASK-026/Rebuild/ReworkV2'
route=np.array(json.loads((source/'s1-route.json').read_text(encoding='utf-8'))['loop'])
index=cKDTree(route[:,:2])
inventory=json.loads((root/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/apply.json').read_text(encoding='utf-8'))['after']
trees=[]
for a in inventory['actors']:
    if a['kind']=='tree':
        trees += [[(a['actor_transform'][0][k]+i['transform'][0][k])/100 for k in range(2)] for i in a['instances']]
tree_index=cKDTree(trees)
rng=np.random.default_rng(stable_seed('S1_CAMP_LAKE',[0,0],'smalltree','1'))
points=[]
for attempt in range(1000):
    center=route[int(rng.integers(30,240))]
    x,y=center[:2]+rng.normal(0,10,2)
    z=sample(h,x,y)
    if not (-1220<x<-740 and -990<y<-510):continue
    if math.hypot(x+980,y+750)<57 or wet(x,y,z) or sample(slope,x,y)>.4:continue
    if index.query([x,y])[0]<4.5 or tree_index.query([x,y])[0]<3.5:continue
    if any(math.dist((x,y),p['xyz_m'][:2])<6 for p in points):continue
    ground=min(sample(h,x+dx,y+dy) for dx,dy in [(0,0),(.4,0),(-.4,0),(0,.4),(0,-.4)])
    points.append({'id':f'S1-smalltree-1-{attempt}','xyz_m':[float(x),float(y),float(ground)],
        'height_m':float(rng.uniform(3.1,4.6)),'yaw':float(rng.uniform(0,360)),
        'cell':[math.floor(x/252),math.floor(y/252)]})
    if len(points)==42:break
assert len(points)==42
(source/'s1-small-trees.json').write_text(json.dumps({'version':'1','points':points},indent=2),encoding='utf-8')
print('Prepared',len(points),'small trees in',len({tuple(p['cell']) for p in points}),'cells')
