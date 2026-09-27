"""Rebuild TASK-049 route with the existing terrain and dressing collision footprints."""
import json,math,heapq,sys
from pathlib import Path
import numpy as np
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'scripts/world/TASK-026'))
import prepare_dressing as terrain
candidate=root/'docs/planning/TASK-049/candidate.json'
data=json.loads(candidate.read_text(encoding='utf-8'))
old=json.loads((root/'docs/world/TASK-049/terrain-route.json').read_text(encoding='utf-8'))
bounds=json.loads((root/'docs/qa/TASK-049/route-obstruction3-pie.json').read_text(encoding='utf-8'))['mesh_bounds']
scatter=json.loads((root/'art_source/TASK-026/Rebuild/scatter.json').read_text(encoding='utf-8'))
step=4;hh=terrain.h[::2,::2];ss=terrain.slope[::2,::2]
axis=np.arange(hh.shape[0])*step-2016
xx,yy=np.meshgrid(axis,axis)
wet=(hh<1)|((np.abs(xx-terrain.river_x(yy))<42)&(hh<terrain.water_height(yy)-.1))
for cx,cy,rx,ry,level in terrain.layout['lakes']:
    angle=np.arctan2((yy-cy)/ry,(xx-cx)/rx)
    distance=np.hypot((xx-cx)/rx,(yy-cy)/ry)*(1+.10*np.sin(3*angle)+.06*np.sin(5*angle+.7)+.03*np.sin(9*angle))
    wet|=(distance<1.02)&(hh<level+.2)
allowed=(ss<.60)&~wet
obstacles=[]
for kind in ['rock','stump','tree']:
    for x,y,z,yaw,height in scatter[kind]:
        if kind=='tree':
            # Existing trunk proxy: radius/height=7.5/100, height=tree height*.32.
            radius=height*.32*.075;cx,cy=x,y
        else:
            b=bounds[kind.title()];scale=height/(2*b['extent'][2]);a=math.radians(yaw)
            cx=x+scale*(b['origin'][0]*math.cos(a)-b['origin'][1]*math.sin(a))
            cy=y+scale*(b['origin'][0]*math.sin(a)+b['origin'][1]*math.cos(a))
            radius=math.hypot(*b['extent'][:2])*scale
        # Capsule clearance plus half the diagonal of a 4m planning cell.
        radius+=4
        obstacles.append((cx,cy,radius))
        x0=max(0,int((cx-radius+2016)//step));x1=min(len(axis),int((cx+radius+2016)//step)+2)
        y0=max(0,int((cy-radius+2016)//step));y1=min(len(axis),int((cy+radius+2016)//step)+2)
        allowed[y0:y1,x0:x1]&=((xx[y0:y1,x0:x1]-cx)**2+(yy[y0:y1,x0:x1]-cy)**2>radius*radius)
def grid(p):return round((p[1]+2016)/step),round((p[0]+2016)/step)
def nearest(p):
    j,i=grid(p)
    if allowed[j,i]:return j,i
    q=[(di*di+dj*dj,(j+dj,i+di)) for di in range(-8,9) for dj in range(-8,9) if allowed[j+dj,i+di]]
    return min(q)[1]
def astar(a,b):
    start,end=nearest(a),nearest(b)
    queue=[(0,start)];cost={start:0};prev={}
    while queue:
        _,p=heapq.heappop(queue)
        if p==end:
            result=[p]
            while p!=start:p=prev[p];result.append(p)
            return [[i*step-2016,j*step-2016,float(hh[j,i])] for j,i in result[::-1]]
        for dj,di in [(0,1),(0,-1),(1,0),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)]:
            q=p[0]+dj,p[1]+di
            if not(0<=q[0]<len(axis) and 0<=q[1]<len(axis)) or not allowed[q]:continue
            if di and dj and (not allowed[p[0],q[1]] or not allowed[q[0],p[1]]):continue
            value=cost[p]+math.hypot(di,dj)*(1+ss[q]*3)
            if value<cost.get(q,1e30):cost[q]=value;prev[q]=p;heapq.heappush(queue,(value+math.dist(q,end),q))
    raise RuntimeError('No dry collision-clear route')
locations={v['id']:v for v in data['locations']}
segments=[astar(locations[a]['xy'],locations[b]['xy']) for a,b in zip(old['waypoints'],old['waypoints'][1:])]
path=[p for s in segments for p in s]
lengths=[sum(math.dist(a,b) for a,b in zip(s,s[1:])) for s in segments]
report={'method':'4m sampled A*, original terrain/water plus conservative rock/stump/trunk collision footprints; physical verification separate','waypoints':old['waypoints'],'length_m':sum(lengths),'segment_m':lengths,'path':path,'obstacle_count':len(obstacles),'anchor_offsets_m':{key:math.dist(locations[key]['xy'],[nearest(locations[key]['xy'])[1]*step-2016,nearest(locations[key]['xy'])[0]*step-2016]) for key in old['waypoints']}}
(root/'docs/world/TASK-049/terrain-route.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for file in [candidate,root/'Resources/Data/gameplay.json']:
    content=json.loads(file.read_text(encoding='utf-8'));c=content if file==candidate else content['campaign'];c['route_trace']=[p[:2] for p in path]
    file.write_text(json.dumps(content,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in report.items() if k!='path'},ensure_ascii=False))
