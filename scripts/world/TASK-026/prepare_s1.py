"""Plan a bounded S1 route and masks against the unchanged authoritative height."""
from pathlib import Path
import hashlib
import heapq
import json
import math
import numpy as np
from scipy.ndimage import distance_transform_edt
from PIL import Image
from prepare_dressing import h, slope, wet

ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'art_source/TASK-026/Rebuild/ReworkV2'
OUT.mkdir(exist_ok=True)
DOC=ROOT/'docs/world/TASK-026/rework-v2'
X0,Y0,X1,Y1=-1230,-1000,-730,-500


def grid(p):return round((p[1]-Y0)/2),round((p[0]-X0)/2)


def position(p):
    j,i=p
    x,y=X0+i*2,Y0+j*2
    return [x,y,float(h[round((y+2016)/2),round((x+2016)/2)])]


def main():
    a0,b0=round((Y0+2016)/2),round((X0+2016)/2)
    hh=h[a0:a0+251,b0:b0+251]
    ss=slope[a0:a0+251,b0:b0+251]
    dry=np.array([[not wet(*position((j,i))) for i in range(251)] for j in range(251)])
    allowed=dry & (ss<.52)

    def route(start,end):
        start,end=grid(start),grid(end)
        assert allowed[start] and allowed[end], (start,end)
        q=[(0,start)]; cost={start:0}; previous={}
        while q:
            _,p=heapq.heappop(q)
            if p==end:
                points=[p]
                while p!=start:p=previous[p];points.append(p)
                return [position(p) for p in points[::-1]]
            for dj,di in [(0,1),(0,-1),(1,0),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)]:
                t=p[0]+dj,p[1]+di
                if not (0<=t[0]<251 and 0<=t[1]<251) or not allowed[t]:continue
                if dj and di and (not allowed[p[0],t[1]] or not allowed[t[0],p[1]]):continue
                c=cost[p]+math.hypot(dj,di)*(1+ss[t]*4)
                if c<cost.get(t,1e20):
                    cost[t]=c;previous[t]=p
                    heapq.heappush(q,(c+math.dist(t,end),t))
        raise ValueError('No dry S1 connection')

    anchors=[[-980,-750],[-1036,-740],[-1120,-750],[-1160,-690],[-1080,-640],
             [-1000,-670],[-914,-690],[-910,-790],[-980,-750]]
    path=[]
    for a,b in zip(anchors,anchors[1:]):
        segment=route(a,b)
        path.extend(segment if not path else segment[1:])
    length=sum(math.dist(a,b) for a,b in zip(path,path[1:]))
    assert 500<=length<=800, length
    mask=np.ones((251,251),dtype=bool)
    for p in path:mask[grid(p)]=False
    clearance=distance_transform_edt(mask)*2
    yy,xx=np.mgrid[Y0:Y1+1:2,X0:X1+1:2]
    edge=np.minimum.reduce([xx-X0,X1-xx,yy-Y0,Y1-yy])
    blend=np.clip(edge/20,0,1)
    patch=(.7+.18*np.sin(xx/11+np.cos(yy/17))+.12*np.sin(yy/5+xx/23))
    density=np.clip((.55-ss)/.3,0,1)*dry*patch
    density*=.22+.78*np.clip((clearance-1.5)/3.5,0,1)
    camp=np.hypot(xx+980,yy+750)
    density*=.45+.55*np.clip((camp-32)/20,0,1)
    # No new height edits: all water, masks and placement continue to share
    # the immutable base until a reviewed bounded patch is actually applied.
    source=ROOT/'art_source/TASK-026/Rebuild/height_m.npy'
    authority={'base':source.relative_to(ROOT).as_posix(),'final':source.relative_to(ROOT).as_posix(),
               'patches':[],'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
               'units':'m','spacing_m':2,'origin_m':[-2016,-2016]}
    (DOC/'height-authority.json').write_text(json.dumps(authority,indent=2),encoding='utf-8')
    (OUT/'s1-route.json').write_text(json.dumps({'version':'S1-1','bounds_m':[X0,Y0,X1,Y1],
        'anchors':anchors,'loop':path,'length_m':length,'validation':'planned; collision walk pending'},indent=2),encoding='utf-8')
    np.save(OUT/'s1-clearance.npy',clearance)
    for name,values in [('S1_Blend',blend),('S1_GrassDensity',density*blend)]:
        full=np.zeros(h.shape,dtype=np.float32)
        full[a0:a0+251,b0:b0+251]=values
        Image.fromarray(np.uint8(np.clip(full,0,1)*255)).save(OUT/(name+'.png'))
    print(json.dumps({'length_m':length,'points':len(path),'height_range':[float(hh.min()),float(hh.max())],
                      'scope':[X0,Y0,X1,Y1]}))


if __name__=='__main__':main()
