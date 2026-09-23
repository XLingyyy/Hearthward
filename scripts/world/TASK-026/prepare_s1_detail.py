"""Bounded second-pass shore clusters and grass islands from saved S1 inputs."""
from pathlib import Path
import copy
import json
import math
import numpy as np
from scipy.spatial import cKDTree
from scipy.ndimage import gaussian_filter
from PIL import Image
from prepare_dressing import h, slope, wet, sample
from rework_contract import stable_seed

root=Path(__file__).resolve().parents[3]
out=root/'art_source/TASK-026/Rebuild/ReworkV2'
inventory=json.loads((root/'docs/qa/evidence/TASK-026/rework-v2/S1-scatter/apply.json').read_text(encoding='utf-8'))['after']
actors={(a['kind'],*a['cell']):a for a in inventory['actors']}
route=cKDTree(np.asarray(json.loads((out/'s1-route.json').read_text(encoding='utf-8'))['loop'])[:,:2])
changes={}
new_rocks=[]
rng=np.random.default_rng(stable_seed('S1_CAMP_LAKE',[0,0],'shore_detail','2'))
for group,(cx,cy) in enumerate([(-1006,-692),(-1071,-631),(-1134,-685)]):
    for n in range(45):
        kind='rock' if n<28 else 'shrub'
        x,y=cx+float(rng.normal(0,4.8)),cy+float(rng.normal(0,3.4))
        z=sample(h,x,y)
        if wet(x,y,z) or sample(slope,x,y)>.42 or math.hypot(x+980,y+750)<54:continue
        size=float(rng.uniform(.12,1.15) if kind=='rock' else rng.uniform(.28,.62))
        radius=size*80.149841/57.101704 if kind=='rock' else size*129.333073/39.597428
        if route.query([x,y])[0]<3+radius:continue
        actor=actors.get((kind,math.floor(x/252),math.floor(y/252)))
        if not actor or actor['ownership']!='Generated':continue
        yaw=float(rng.uniform(0,360));angle=math.radians(yaw)
        if kind=='rock':
            low,height,rx,ry=.035519,57.101704,80.149841,34.187942
        else:low,height,rx,ry=-.875256,39.597428,129.333073,10.928048
        scale=size*100/height
        if kind=='rock':
            support=[sample(h,x+u*math.cos(angle)-v*math.sin(angle),y+u*math.sin(angle)+v*math.cos(angle))
                for u in [-rx*scale/100,0,rx*scale/100] for v in [-ry*scale/100,0,ry*scale/100]]
            z=min(support)-size*.12
            new_rocks.append([x,y,radius])
        transform=[[x*100-actor['actor_transform'][0][0],y*100-actor['actor_transform'][0][1],z*100-low*scale-3],
                   [0,0,math.sin(angle/2),math.cos(angle/2)],[scale]*3]
        changes.setdefault(actor['id'],copy.deepcopy(actor['instances'])).append(
            {'id':actor['id']+f':S1-2:shore-{group}-{n}','transform':transform})
(out/'s1-detail-replacements.json').write_text(json.dumps(changes,separators=(',',':')),encoding='utf-8')

yy,xx=np.mgrid[-1000:-498:2,-1230:-728:2]
trees=[]
for a in inventory['actors']:
    if a['kind']=='tree':
        trees.extend([(a['actor_transform'][0][0]+i['transform'][0][0])/100,
                      (a['actor_transform'][0][1]+i['transform'][0][1])/100] for i in a['instances'])
distance=cKDTree(trees).query(np.column_stack([xx.ravel(),yy.ravel()]))[0].reshape(xx.shape)
forest=np.clip((13-distance)/10,0,1)
noise=gaussian_filter(rng.random(xx.shape),sigma=4)
noise=(noise-noise.min())/(noise.max()-noise.min())
islands=np.clip((noise-.3)/.4,0,1)
factor=(.15+.85*islands)*(1-.62*forest)
factor*=np.clip((distance-1.2)/2.8,0,1)
for x,y,radius in new_rocks:
    factor*=np.clip((np.hypot(xx-x,yy-y)-radius*.8)/2.5,0,1)
density=np.asarray(Image.open(out/'S1_GrassDensity.png').convert('L')).copy()
density[508:759,393:644]=np.uint8(density[508:759,393:644]*factor)
Image.fromarray(density).save(out/'S1_GrassDensity_Detail.png')
surface=np.asarray(Image.open(out/'S1_Surface.png').convert('RGBA')).copy()
soil=surface[508:759,393:644,0]/255.
soil=np.maximum(soil,.65*(1-islands)*(.35+.65*forest))
for x,y,radius in new_rocks:
    soil=np.maximum(soil,.8*np.clip((radius+3-np.hypot(xx-x,yy-y))/3,0,1))
surface[508:759,393:644,0]=np.uint8(soil*255)
Image.fromarray(surface).save(out/'S1_Surface_Detail.png')
(out/'s1-detail.json').write_text(json.dumps({'scope_m':[-1230,-1000,-730,-500],
    'batches':len(changes),'added_instances':sum(len(v)-len(next(a for a in inventory['actors'] if a['id']==k)['instances']) for k,v in changes.items()),
    'shore_rock_count':len(new_rocks),'shore_centres_m':[[-1006,-692],[-1071,-631],[-1134,-685]],
    'route_collision_clearance_m':3,'camp_clear_radius_m':54,'grass_island_smoothing_m':8,
    'height_changes':False,'outside_instances_preserved':True},indent=2),encoding='utf-8')
print((out/'s1-detail.json').read_text(encoding='utf-8'))
