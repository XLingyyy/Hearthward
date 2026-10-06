"""Render the existing full-world terrain atlas for the 1 km radius map.

Uses the authored landscape heightmap, water GLB triangles, scatter positions and
current runtime building bounds. The UI reveals a circular region of this atlas.
No terrain or game asset is authored or moved by this script.
"""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image
import draw_local_map as geom
from realistic_local_map import render_realistic

GAME=Path(__file__).resolve().parents[2]
SOURCE=GAME/'art_source/TASK-026/Rebuild'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('scene',type=Path)
    parser.add_argument('--output',type=Path,default=GAME/'Resources/UI/Art/map-local-terrain.png')
    args=parser.parse_args()
    live=json.loads(args.scene.read_text('utf-8'))
    # Verify the height source against the current game's actual collision samples.
    authored=np.load(SOURCE/'height_m.npy')
    actual=np.asarray(live['heightsM'],dtype=float).reshape(live['samples'],live['samples'])
    yy,xx=np.mgrid[:live['samples'],:live['samples']]
    fx=(live['originM'][0]+xx*live['spacingM']+2016)/2
    fy=(live['originM'][1]+yy*live['spacingM']+2016)/2
    ix=np.floor(fx).astype(int);iy=np.floor(fy).astype(int)
    tx=fx-ix;ty=fy-iy
    expected=(authored[iy,ix]*(1-tx)+authored[iy,ix+1]*tx)*(1-ty)+(authored[iy+1,ix]*(1-tx)+authored[iy+1,ix+1]*tx)*ty
    residual=actual-expected
    offset=float(np.nanmedian(residual))
    error=float(np.nanpercentile(np.abs(residual-offset),99))
    if not np.all(np.isfinite(actual)) or error>.3:
        raise ValueError(f'Height source does not match current collision: p99 {error}m')
    size=2400;origin=np.array([-2016.,-2016.]);span=4032.
    # The imported landscape heightfield has a fixed elevation origin; bind it to observations.
    terrain=authored.astype(np.float32)+offset
    small=terrain[::4,::4]
    heights=np.asarray(Image.fromarray(terrain).resize((size,size),Image.Resampling.BICUBIC))
    scene=dict(samples=small.shape[0],spacingM=8,heightsM=small.ravel().tolist(),houses=live['houses'],trees=[],rocks=[],water=[])
    scatter=json.loads((SOURCE/'scatter.json').read_text('utf-8'))
    for name,key in [('tree','trees'),('rock','rocks')]:
        scene[key]=[dict(x=p[0],y=p[1]) for p in scatter.get(name,[])]
    # Live instanced footprints override the authored positions in the loaded opening area.
    for key in ('trees','rocks'):
        scene[key]=[p for p in scene[key] if np.hypot(p['x']-920,p['y']-535)>350]+live[key]
    observed={p['mesh'].removeprefix('SM_'):p for p in live['water']}
    for row in json.loads((SOURCE/'water.json').read_text('utf-8')):
        name=row['name'];p=observed.get(name)
        if p is None:
            vertices,_=geom.glb(SOURCE/'water'/(name+'.glb'))
            xy=vertices[:,[0,2]]+np.asarray(row['position'][:2])
            p=dict(x=row['position'][0],y=row['position'][1],z=row['position'][2],mesh='SM_'+name,scale=[1,1,1],boundsM=[*xy.min(axis=0).tolist(),*xy.max(axis=0).tolist()])
        scene['water'].append(p)
    geom.SIZE=size;geom.ORIGIN=origin;geom.SPAN=span
    levels,mapping=geom.surface_levels(scene)
    image,style=render_realistic(scene,heights,levels,GAME/'Resources/UI/Art/map-surface-materials.png',origin,span,size,clip_circle=False)
    image.save(args.output)
    manifest=dict(resolution=[size,size],terrain_origin_m=origin.tolist(),terrain_span_m=span,radius_m=1000,
        elevation_offset_m=offset,collision_p99_error_m=error,collision_samples=int(actual.size),water_mapping=mapping,
        houses=len(scene['houses']),trees=len(scene['trees']),rocks=len(scene['rocks']),
        scene_sha256=hashlib.sha256(args.scene.read_bytes()).hexdigest(),height_source_sha256=hashlib.sha256((SOURCE/'height_m.npy').read_bytes()).hexdigest(),
        output_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),**style)
    (args.scene.parent/'cartography.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False),encoding='utf-8')
    print(json.dumps(manifest,ensure_ascii=False),flush=True)

if __name__=='__main__':main()
