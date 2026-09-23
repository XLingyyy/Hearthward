"""Deterministic slope-aware placement and native-importable water meshes."""
from pathlib import Path
import json
import math
import heapq
import numpy as np
from scipy.ndimage import distance_transform_edt
import trimesh
from PIL import Image, ImageDraw
from prepare_rebuild import OUT, DOC, river_x, water_height

h=np.load(OUT/'height_m.npy')
layout=json.loads((OUT/'layout.json').read_text(encoding='utf-8'))
dy,dx=np.gradient(h,2)
slope=np.hypot(dx,dy)


def sample(a,x,y):
    return float(a[min(2016,max(0,round((y+2016)/2))),min(2016,max(0,round((x+2016)/2)))])


def wet(x,y,z):
    if z<1:return True
    if abs(x-river_x(y))<42 and z<float(water_height(y))-.10:return True
    for cx,cy,rx,ry,level in layout['lakes']:
        a=math.atan2((y-cy)/ry,(x-cx)/rx)
        d=math.hypot((x-cx)/rx,(y-cy)/ry)*(1+.10*math.sin(3*a)+.06*math.sin(5*a+.7)+.03*math.sin(9*a))
        if d<1.02 and z<level+.2:return True
    return False


def create_route():
    if (OUT / 'routes.json').exists():
        raise RuntimeError('Existing route baseline must be versioned; initial generation cannot replace it.')
    step=4
    hh=h[::step,::step]; ss=slope[::step,::step]
    allowed=ss<.60
    for j in range(hh.shape[0]):
        for i in range(hh.shape[1]):
            if wet(i*8-2016,j*8-2016,float(hh[j,i])):allowed[j,i]=False
    def grid(p):return (round((p[1]+2016)/8),round((p[0]+2016)/8))
    def astar(start,end):
        start,end=grid(start),grid(end)
        assert allowed[start] and allowed[end], ('route endpoint not walkable',start,end)
        queue=[(0,start)]; cost={start:0}; previous={}
        while queue:
            _,p=heapq.heappop(queue)
            if p==end:
                path=[p]
                while p!=start:p=previous[p];path.append(p)
                return [(i*8-2016,j*8-2016,float(hh[j,i])) for j,i in path[::-1]]
            for dj,di in [(0,1),(0,-1),(1,0),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)]:
                q=(p[0]+dj,p[1]+di)
                if not(0<=q[0]<hh.shape[0] and 0<=q[1]<hh.shape[1]) or not allowed[q]:continue
                if dj and di and (not allowed[p[0],q[1]] or not allowed[q[0],p[1]]):continue
                c=cost[p]+math.hypot(di,dj)*(1+ss[q]*3)
                if c<cost.get(q,1e20):
                    cost[q]=c;previous[q]=p
                    heapq.heappush(queue,(c+math.hypot(q[0]-end[0],q[1]-end[1]),q))
        raise RuntimeError(('No walkable route',start,end))
    # The loop traverses forest, northern pass, meadow, both river crossings.
    points=[(-980,-750),(-1216,496),(-600,700),(float(river_x(680)),680),(1090,450),
            (1100,-1120),(float(river_x(-1120)),-1120),(-700,-950),(-980,-750)]
    segments=[astar(a,b) for a,b in zip(points,points[1:])]
    loop=[p for s in segments for p in s]
    branches=[astar((-1216,496),(-1050,1000)),astar((1090,450),(1430,800))]
    allpoints=loop+[p for b in branches for p in b]
    mask=np.ones(h.shape,dtype=bool)
    for x,y,z in allpoints:mask[round((y+2016)/2),round((x+2016)/2)]=False
    clearance=distance_transform_edt(mask)*2
    route={'loop':loop,'branches':branches,'length_m':sum(math.dist(a,b) for a,b in zip(loop,loop[1:])),
           'method':'8m A* across <=31 degree sampled dry terrain; pending UE walking verification'}
    (OUT/'routes.json').write_text(json.dumps(route),encoding='utf-8')
    plan=Image.open(DOC/'rebuild-masterplan.png');d=ImageDraw.Draw(plan)
    for path,col in [(loop,'#eadbb0')]+[(b,'#ffad6e') for b in branches]:
        d.line([((x+2016)/4032*1200,(y+2016)/4032*1200) for x,y,z in path],fill=col,width=3)
    plan.save(DOC/'rebuild-masterplan.png')
    print('Planned loop length:',round(route['length_m']),flush=True)
    return clearance


def write_mesh(name,verts,faces):
    # Axis exchange changes handedness; reverse winding to keep the water top-facing.
    mesh=trimesh.Trimesh(vertices=np.asarray(verts)[:,[0,2,1]],faces=np.asarray(faces)[:,::-1],process=False)
    path=OUT/'water';path.mkdir(exist_ok=True)
    mesh.export(path/(name+'.glb'))


def water_meshes():
    if (OUT / 'water.json').exists():
        raise RuntimeError('Existing water topology must use an explicit affected-segment update.')
    manifest=[]
    for j in range(16):
        y0=-2016+j*252; yc=y0+126; xc=float(river_x(yc));zc=float(water_height(yc))
        verts=[];faces=[]
        for n,y in enumerate(np.linspace(y0,y0+252,64)):
            width=27+9*math.cos(y/340)
            for side in [-1,1]:verts.append([float(river_x(y))+side*width-xc,float(y)-yc,float(water_height(y))-zc])
            if n:faces.extend([[2*n-2,2*n-1,2*n],[2*n-1,2*n+1,2*n]])
        name=f'River_{j:02}'
        write_mesh(name,verts,faces)
        manifest.append({'name':name,'position':[xc,yc,zc],'kind':'falls' if -800<yc<-500 else 'water'})
    for j,(cx,cy,rx,ry,z) in enumerate(layout['lakes']):
        verts=[[0,0,0]];faces=[]
        for k in range(97):
            a=math.tau*k/96
            r=1.02/(1+.10*math.sin(3*a)+.06*math.sin(5*a+.7)+.03*math.sin(9*a))
            verts.append([rx*math.cos(a)*r,ry*math.sin(a)*r,0])
            if k:faces.append([0,k,k+1])
        name=f'Lake_{j}';write_mesh(name,verts,faces)
        manifest.append({'name':name,'position':[cx,cy,z],'kind':'water'})
    write_mesh('Sea',[[0,-3200,0],[4000,-3200,0],[0,3200,0],[4000,3200,0]],[[0,1,2],[1,3,2]])
    manifest.append({'name':'Sea','position':[1450,0,0],'kind':'water'})
    (OUT/'water.json').write_text(json.dumps(manifest),encoding='utf-8')
    verts=[]; faces=[];uv=[]
    for angle in [0,math.pi/3,math.pi*2/3]:
        vx,vy=.35*math.cos(angle),.35*math.sin(angle)
        offset=len(verts)
        verts.extend([[-vx,0,-vy],[vx,0,vy],[vx,1,vy],[-vx,1,-vy]])
        # TASK-004 texture is an atlas. Use the upright lower-left grass tuft,
        # with OpenGL V coordinates (trimesh flips V when exporting glTF).
        uv.extend([[.20,.09],[.52,.09],[.52,.245],[.20,.245]])
        faces.extend([[offset,offset+1,offset+2],[offset,offset+2,offset+3]])
    mesh=trimesh.Trimesh(vertices=verts,faces=faces,process=False)
    mesh.visual=trimesh.visual.texture.TextureVisuals(uv=np.array(uv))
    mesh.export(OUT/'water/GrassCards.glb')
    # Hidden collision-only proxy, scaled to each tree; never used as scenery.
    trunk=trimesh.creation.cylinder(radius=.075,height=1,sections=12)
    trunk.vertices[:,2]+=.5
    write_mesh('TrunkCollision',trunk.vertices,trunk.faces)
    yy,xx=np.mgrid[0:512,0:512]/512*math.tau
    nx=.18*np.cos(xx*9+np.sin(yy*3))+.07*np.cos(xx*23-yy*13)
    ny=.16*np.sin(yy*11+np.cos(xx*4))+.07*np.sin(xx*23-yy*13)
    nz=np.sqrt(1-nx*nx-ny*ny)
    Image.fromarray(np.uint8((np.stack([nx,ny,nz],axis=-1)+1)*127.5)).save(OUT/'water_normal.png')


def scatter(clearance):
    if (OUT / 'scatter.json').exists():
        raise RuntimeError('Existing scatter requires a reviewed whole-batch ReworkV2 plan.')
    rng=np.random.default_rng(260923)
    groups={k:[] for k in ['tree','shrub','rock','stump','grass']}
    for kind,count in [('tree',80000),('shrub',22000),('rock',8500),('stump',300),('grass',140000)]:
        for _ in range(count):
            x,y=rng.uniform(-1950,1630),rng.uniform(-1950,1950)
            z=sample(h,x,y); s=sample(slope,x,y)
            if wet(x,y,z) or sample(clearance,x,y)<(8 if kind in ['tree','rock'] else 3):continue
            if any(math.hypot(x-cx,y-cy)<r for cx,cy,r,_ in layout['clearings']):continue
            forest=x<(-210+80*math.sin(y/320))
            if kind in ['tree','shrub','stump']:
                if z>580 or s>.90:continue
                density=(.87 if forest else .075)*(.70+.30*math.sin(x/95)*math.cos(y/120))
                if rng.random()>density:continue
            if kind=='grass' and (s>.45 or z>430 or (forest and rng.random()<.65)):continue
            if kind=='rock' and s<.26 and rng.random()<.76:continue
            height={'tree':rng.uniform(13,23),'shrub':rng.uniform(.7,2),'rock':rng.uniform(1.0,5),
                    'stump':rng.uniform(.6,1.4),'grass':rng.uniform(.25,.7)}[kind]
            if kind=='rock' and s>.55 and rng.random()<.4:height=rng.uniform(9,27)
            groups[kind].append([round(float(v),3) for v in [x,y,z,float(rng.uniform(0,360)),height]])
    # A wide scan on a slope needs support across its footprint, not just its pivot.
    contacts=[]
    for x,y,z,yaw,height in groups['rock']:
        a=math.radians(yaw)
        rx=height*80.149841/57.101704*.9;ry=height*34.187942/57.101704*.9
        contacts.append(min(float(sample(h,x+u*math.cos(a)-v*math.sin(a),y+u*math.sin(a)+v*math.cos(a)))
                            for u in [-rx,0,rx] for v in [-ry,0,ry])-height*.15)
    (OUT/'rock_contact_heights.json').write_text(json.dumps(contacts),encoding='utf-8')
    (OUT/'scatter.json').write_text(json.dumps(groups,separators=(',',':')),encoding='utf-8')
    print('Placement counts:',{k:len(v) for k,v in groups.items()},flush=True)
    yy,xx=np.mgrid[0:2017,0:2017]*2-2016
    mask=np.clip((.65-slope)/.25,0,1)*np.clip((500-h)/100,0,1)
    mask[h<1]=0
    water=(np.abs(xx-river_x(yy))<42)&(h<water_height(yy)+.2)
    for cx,cy,rx,ry,level in layout['lakes']:
        a=np.arctan2((yy-cy)/ry,(xx-cx)/rx)
        d=np.hypot((xx-cx)/rx,(yy-cy)/ry)*(1+.10*np.sin(3*a)+.06*np.sin(5*a+.7)+.03*np.sin(9*a))
        water|=(d<1.02)&(h<level+.2)
    mask[water]=0
    mask*=np.where(clearance<4,.08,1)
    for cx,cy,r,_ in layout['clearings']:mask*=np.where(np.hypot(xx-cx,yy-cy)<r,.15,1)
    Image.fromarray(np.uint8(mask*255)).save(OUT/'grass_density.png')


if __name__=='__main__':
    clearance=create_route()
    scatter(clearance)
    water_meshes()
