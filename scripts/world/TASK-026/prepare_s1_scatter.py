"""Prepare complete existing batches for S1; keep every outside instance intact."""
from pathlib import Path
import copy
import json
import math
import numpy as np
from scipy.spatial import cKDTree
from prepare_dressing import h, slope, wet, sample
from rework_contract import stable_seed, digest

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'art_source/TASK-026/Rebuild/ReworkV2'
inventory=json.loads((ROOT/'docs/qa/evidence/TASK-026/rework-v2/R1-density-repeat/apply.json').read_text(encoding='utf-8'))['after']
route=json.loads((OUT/'s1-route.json').read_text(encoding='utf-8'))
points=np.asarray(route['loop'])[:,:2]
route_tree=cKDTree(points)
actors={(a['kind'],*a['cell']):a for a in inventory['actors']}
changes={}
counts={'added':{},'removed':{}}
# Native mesh bounds recorded by rebuild/dressing.json, in cm.
bounds={'tree':(-14.661538,1946.88749,1220.852478,957.728699),
        'shrub':(-.875256,39.597428,129.333073,10.928048),
        'rock':(.035519,57.101704,80.149841,34.187942),
        'trunk':(0,100,7.5,7.5)}

def inside(x,y):return -1230<=x<=-730 and -1000<=y<=-500
def world(a,i):return [(a['actor_transform'][0][k]+i['transform'][0][k])/100 for k in range(3)]
def distance(x,y):return float(route_tree.query([x,y])[0])
def batch(kind,x,y):return actors.get((kind,math.floor(x/252),math.floor(y/252)))
def writable(a):
    assert a['ownership']=='Generated'
    return changes.setdefault(a['id'],copy.deepcopy(a['instances']))

# Clear only real collision footprints near the sample route; accent grass
# is thinned now that Landscape Grass supplies the continuous ground cover.
for a in inventory['actors']:
    if a['ownership']!='Generated':continue
    keep=[]
    for i in a['instances']:
        x,y,z=world(a,i);remove=False
        if inside(x,y):
            kind=a['kind'];d=distance(x,y)
            if kind=='trunk':remove=d<2.8
            elif kind=='tree':remove=d<2.8
            elif kind=='rock':remove=d<2+80.149841*i['transform'][2][0]/100
            elif kind=='grass':remove=int(digest(i['id'])[:8],16)%8!=0
        if remove:counts['removed'][a['kind']]=counts['removed'].get(a['kind'],0)+1
        else:keep.append(i)
    if len(keep)!=len(a['instances']):changes[a['id']]=keep

existing_trees=[]
for a in inventory['actors']:
    if a['kind']=='tree':existing_trees.extend(world(a,i)[:2] for i in changes.get(a['id'],a['instances']) if inside(*world(a,i)[:2]))

def add(kind,x,y,desired,yaw,token):
    a=batch(kind,x,y)
    if not a:return False
    low,height,rx,ry=bounds[kind];scale=desired*100/height
    z=sample(h,x,y)
    if kind=='rock':
        angle=math.radians(yaw)
        support=[sample(h,x+u*math.cos(angle)-v*math.sin(angle),y+u*math.sin(angle)+v*math.cos(angle))
                 for u in [-rx*scale/100,0,rx*scale/100] for v in [-ry*scale/100,0,ry*scale/100]]
        z=min(support)-desired*.1
    elif kind=='tree':
        z=min(sample(h,x+dx,y+dy) for dx,dy in [(0,0),(.5,0),(-.5,0),(0,.5),(0,-.5)])
    angle=math.radians(yaw)/2
    transform=[[round(x*100-a['actor_transform'][0][0],6),round(y*100-a['actor_transform'][0][1],6),round(z*100-low*scale-3,6)],
               [0,0,math.sin(angle),math.cos(angle)],[scale]*3]
    writable(a).append({'id':a['id']+':S1-1:'+token,'transform':transform})
    counts['added'][kind]=counts['added'].get(kind,0)+1
    return True

rng=np.random.default_rng(stable_seed('S1_CAMP_LAKE',[0,0],'composition','1'))
# Irregular wooded edge around the usable 50m camp core. The two route exits
# remain narrow walkable openings, and the north-east lake sightline is open.
for n in range(650):
    x,y=rng.uniform(-1210,-750),rng.uniform(-980,-520)
    z=sample(h,x,y);s=sample(slope,x,y);r=math.hypot(x+980,y+750)
    angle=math.atan2(y+750,x+980)
    edge=64+9*math.sin(angle*3)+5*math.cos(angle*5)
    if r<edge or wet(x,y,z) or s>.52 or distance(x,y)<3.2:continue
    if r>155:continue
    if y>-715 and x>-1030:continue
    if existing_trees and min(math.dist((x,y),p) for p in existing_trees)<9:continue
    desired=float(rng.uniform(11,19));yaw=float(rng.uniform(0,360))
    if not batch('tree',x,y) or not batch('trunk',x,y):continue
    add('tree',x,y,desired,yaw,str(n));add('trunk',x,y,desired*.32,yaw,str(n))
    existing_trees.append([x,y])

# Understorey clusters follow the existing and newly shaped forest edge.
for n in range(650):
    if not existing_trees:break
    cx,cy=existing_trees[int(rng.integers(len(existing_trees)))]
    x,y=cx+float(rng.normal(0,5)),cy+float(rng.normal(0,5))
    if not inside(x,y) or math.hypot(x+980,y+750)<52 or distance(x,y)<3:continue
    z=sample(h,x,y)
    if wet(x,y,z) or sample(slope,x,y)>.5:continue
    add('shrub',x,y,float(rng.uniform(.22,.48)),float(rng.uniform(0,360)),str(n))

# Three deliberately placed rock groupings: slope outcrop, forest turn,
# and low shore stones. Support samples use the full rotated mesh footprint.
for group,(cx,cy,count,lo,hi,spread) in enumerate([(-1176,-706,12,1.4,4.5,13),(-1116,-732,9,.6,2.3,9),(-1049,-648,18,.18,.65,14)]):
    for n in range(count):
        x,y=cx+float(rng.normal(0,spread)),cy+float(rng.normal(0,spread))
        size=float(rng.uniform(lo,hi));z=sample(h,x,y)
        if not inside(x,y) or wet(x,y,z) or distance(x,y)<2+size*80.149841/57.101704:continue
        add('rock',x,y,size,float(rng.uniform(0,360)),f'{group}-{n}')

# A complete batch includes the exact untouched records outside S1.
lookup={a['id']:a for a in inventory['actors']}
for key,items in changes.items():
    a=lookup[key]
    assert [i for i in a['instances'] if not inside(*world(a,i)[:2])]==[i for i in items if not inside(*world(a,i)[:2])]
(OUT/'s1-replacements.json').write_text(json.dumps(changes,separators=(',',':')),encoding='utf-8')
(OUT/'s1-scatter-summary.json').write_text(json.dumps({'version':'S1-1','complete_batches':len(changes),'counts':counts,
    'outside_instances_unchanged':True,'height_edits':False,'protected_authored_untouched':True},indent=2),encoding='utf-8')
print(json.dumps({'batches':len(changes),**counts}))
