"""Audit uncovered map captures and compare actual ground/buildings to atlas inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image

GAME=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--atlas-scene',type=Path,required=True)
    args=parser.parse_args();run=args.run
    config=json.loads((GAME/'Resources/UI/interface.json').read_text('utf-8'))
    m=config['localMap'];art=Image.open(GAME/'Resources/UI/Art/map-local-terrain.png')
    manifest=json.loads((run/'source-manifest.json').read_text('utf-8'))
    mismatch=[n for n,h in manifest['fingerprints'].items() if hashlib.sha256((GAME/n).read_bytes()).hexdigest()!=h]
    report=json.loads((run/'report.json').read_text('utf-8'))
    checks=dict(current_fingerprints_match=not mismatch,runtime_checks_pass=report['passed'],
        rectangle_is_3000_by_2000_metres=m['sizeCm']==[300000,200000],
        no_fog_or_circle_config='outlineFractions' not in m and 'fogStyle' not in m and 'mapDarkFog' not in config['assets'],
        whole_atlas_uv=config['assets']['mapLocalTerrain']['uv']==[0,0,art.width,art.height])
    scene=json.loads((run/'scene.json').read_text('utf-8'))
    probes=scene['rectangleHeightProbes']
    field=np.load(GAME/'art_source/TASK-026/Rebuild/height_m.npy')
    xs=np.array([p['xM'] for p in probes]);ys=np.array([p['yM'] for p in probes])
    fx=(xs+2016)/2;fy=(ys+2016)/2
    ix=np.floor(fx).astype(int);iy=np.floor(fy).astype(int);tx=fx-ix;ty=fy-iy
    expected=(field[iy,ix]*(1-tx)+field[iy,ix+1]*tx)*(1-ty)+(field[iy+1,ix]*(1-tx)+field[iy+1,ix+1]*tx)*ty
    found=all(p['found'] for p in probes)
    actual=np.array([p.get('heightM',np.nan) for p in probes]);offset=float(np.nanmedian(actual-expected))
    p99=float(np.nanpercentile(np.abs(actual-expected-offset),99))
    checks['all_651_distributed_probes_hit_actual_ground']=len(probes)==651 and found
    checks['full_rectangle_matches_authored_height_source']=found and p99<.3 and abs(offset)<.1
    old=json.loads(args.atlas_scene.read_text('utf-8'))
    def footprints(rows):
        return sorted((p.get('part',''),*(round(p[k],4) for k in ['x','y','halfX','halfY'])) for p in rows)
    checks['all_building_footprints_match_current_game']=footprints(scene['houses'])==footprints(old['houses'])
    center=np.array(scene['mapView']['regionCenterCm']);size=np.array(m['sizeCm']);origin=np.array(m['terrainOriginCm'])
    checks['rectangle_within_actual_terrain_extent']=bool(np.all(center-size/2>=origin) and np.all(center+size/2<=origin+m['terrainSpanCm']))
    states={}
    for name in ['reveal-1600x1000','reveal-1920x1080','reveal-1280x720','reveal-ultrawide','reveal-zoom-pan','reveal-minimum-zoom']:
        base=np.asarray(Image.open(run/(name+'-base.png')).convert('RGBA')).astype(int)
        probe=np.asarray(Image.open(run/(name+'-probe.png')).convert('RGBA')).astype(int)
        data=json.loads((run/(name+'-base.json')).read_text('utf-8'));view=data['mapView']
        height,width=base.shape[:2];screen_scale=min(width/1672,height/941)
        offset_px=(np.array([width,height])-np.array([1672,941])*screen_scale)/2
        yy,xx=np.mgrid[:height,:width]
        x=(xx+.5-offset_px[0])/screen_scale;y=(yy+.5-offset_px[1])/screen_scale
        vp=np.array(view['viewport']);point=(vp[:2]+vp[2:])/2+np.array(view['pan'])
        half=np.array(view['sizeCm'])*view['scaleDesignUnitsPerCm']/2
        lo=np.maximum(vp[:2],point-half);hi=np.minimum(vp[2:],point+half)
        interior=(x>lo[0]+3)&(x<hi[0]-3)&(y>lo[1]+3)&(y<hi[1]-3)
        exterior=(x<lo[0]-3)|(x>hi[0]+3)|(y<lo[1]-3)|(y>hi[1]+3)
        delta=np.max(np.abs(base[...,:3]-probe[...,:3]),axis=2)
        fraction=float(np.mean(delta[interior]>20));outside=int(delta[exterior].max()) if np.any(exterior) else 0
        edge=(xx<8)|(xx>=width-8)|(yy<8)|(yy>=height-8)
        edge_fraction=float(np.mean(delta[edge]>20))
        screen_lo=vp[:2]*screen_scale+offset_px;screen_hi=vp[2:]*screen_scale+offset_px
        states[name]=dict(uncovered_rectangle_fraction=fraction,uncovered_screen_edge_fraction=edge_fraction,
            max_outside_difference=outside,resolution=[width,height])
        checks[name+'_rectangle_completely_revealed']=fraction>.97
        checks[name+'_no_content_leaks_outside_rectangle']=outside<=1
        checks[name+'_opaque_viewport']=bool(np.all(base[...,3]==255))
        checks[name+'_map_viewport_fills_screen']=bool(np.all(np.abs(screen_lo)<.1) and np.all(np.abs(screen_hi-[width,height])<.1))
        checks[name+'_terrain_covers_all_screen_edges']=edge_fraction>.97
    elements=report['states']['initial']['elements']
    checks['floating_location_toggle_retained']=any(e['id']=='map.locations.toggle' and e['action']=='map.world' for e in elements)
    checks['surrounding_frame_and_descriptions_removed']=not any(e['id']=='map.frame' or e['type']=='mapFrame'
        or any(t in e['text'] for t in ['3000米','Esc 返回','按住左键拖动']) for e in elements)
    checks['geographic_labels_retained']=any(e['type']=='mapRegion' and e['visible'] for e in elements)
    for label in ['initial','minimum_zoom']:
        v=report['states'][label]['mapView'];vp=np.array(v['viewport']);extent=np.array(v['sizeCm'])*v['scaleDesignUnitsPerCm']
        checks[label+'_view_extent']=bool(np.any(extent>vp[2:]-vp[:2]+1)) if label=='initial' else bool(np.all(extent>=vp[2:]-vp[:2]-.1))
    out=dict(passed=all(checks.values()),checks=checks,mismatches=mismatch,states=states,
        geography=dict(probes=len(probes),height_p99_error_m=p99,height_offset_m=offset,building_components=len(scene['houses']),
            height_source_sha256=hashlib.sha256((GAME/'art_source/TASK-026/Rebuild/height_m.npy').read_bytes()).hexdigest(),
            current_scene_sha256=hashlib.sha256((run/'scene.json').read_bytes()).hexdigest(),
            atlas_scene_sha256=hashlib.sha256(args.atlas_scene.read_bytes()).hexdigest(),
            atlas_sha256=hashlib.sha256((GAME/'Resources/UI/Art/map-local-terrain.png').read_bytes()).hexdigest()),
        scope='Real current standalone Widget captures, distributed real collision heights and exact current building footprints. Not physical mouse input or full gameplay acceptance.')
    (run/'rectangular-audit.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(out,ensure_ascii=False),flush=True)
    raise SystemExit(0 if out['passed'] else 1)

if __name__=='__main__':main()
