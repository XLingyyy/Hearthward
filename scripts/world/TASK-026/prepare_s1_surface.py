"""Generate bounded forest-floor, shore and macro masks from final S1 inputs."""
from pathlib import Path
import json
import numpy as np
from scipy.spatial import cKDTree
from scipy.ndimage import distance_transform_edt, gaussian_filter
from PIL import Image
from prepare_dressing import h, wet

root=Path(__file__).resolve().parents[3]
out=root/'art_source/TASK-026/Rebuild/ReworkV2'
inventory=json.loads((root/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/apply.json').read_text())['after']
trees=[]
for a in inventory['actors']:
    if a['kind']!='tree':continue
    origin=a['actor_transform'][0]
    trees.extend([(origin[0]+i['transform'][0][0])/100,(origin[1]+i['transform'][0][1])/100] for i in a['instances'])
yy,xx=np.mgrid[-1000:-498:2,-1230:-728:2]
j0,i0=508,393
hh=h[j0:j0+251,i0:i0+251]
distance=cKDTree(trees).query(np.column_stack([xx.ravel(),yy.ravel()]))[0].reshape(xx.shape)
forest=np.clip((13-distance)/10,0,1)
dry=np.array([[not wet(float(x),float(y),float(z)) for x,y,z in zip(rx,ry,rz)] for rx,ry,rz in zip(xx,yy,hh)])
shore=distance_transform_edt(dry)*2
moisture=np.clip((14-shore)/14,0,1)*np.clip((163-hh)/6,0,1)
rng=np.random.default_rng(260202)
noise=gaussian_filter(rng.random(xx.shape),sigma=7)
noise=(noise-noise.min())/(noise.max()-noise.min())
macro=gaussian_filter(rng.random(xx.shape),sigma=17)
macro=(macro-macro.min())/(macro.max()-macro.min())
soil=np.clip(.12+.48*forest+.32*moisture+.25*(noise-.5),0,1)
local=np.stack([soil,moisture,forest,macro],axis=-1)
full=np.zeros((*h.shape,4),dtype=np.uint8)
full[j0:j0+251,i0:i0+251]=np.uint8(np.clip(local,0,1)*255)
Image.fromarray(full).save(out/'S1_Surface.png')
(out/'s1-surface.json').write_text(json.dumps({'channels':{'R':'soil blend','G':'shore moisture','B':'tree proximity forest floor','A':'low contrast macro'},
    'scope_m':[-1230,-1000,-730,-500],'height_source':'height_m.npy unchanged',
    'scatter_source':'S1-scatter/apply.json after','fine_texture_period_m':5,
    'macro_smoothing_sigma_m':34,'shore_extent_m':14,'forest_fade_m':[3,13]},indent=2),encoding='utf-8')
