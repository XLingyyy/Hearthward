"""Verify current map fingerprints, full-atlas UVs and rendered fog opacity."""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image
from audit_local_map import boundary_fraction,contour_field
GAME=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path);args=parser.parse_args();run=args.run
    config=json.loads((GAME/'Resources/UI/interface.json').read_text('utf-8'));m=config['localMap']
    manifest=json.loads((run/'source-manifest.json').read_text('utf-8'))
    mismatches=[name for name,value in manifest['fingerprints'].items() if hashlib.sha256((GAME/name).read_bytes()).hexdigest()!=value]
    report=json.loads((run/'report.json').read_text('utf-8'))
    art=Image.open(GAME/'Resources/UI/Art/map-local-terrain.png')
    checks=dict(current_fingerprints_match=not mismatches,all_runtime_checks_pass=report['passed'],radius_is_1000m=m['radiusCm']==100000,
        atlas_uv_covers_entire_image=config['assets']['mapLocalTerrain']['uv']==[0,0,art.width,art.height],equal_xy_scale=m['terrainSpanCm']==403200,
        legacy_page_removed='worldMapPage' not in config)
    outline=np.array(m['outlineFractions']);angular=m['outlineAngularSegments']
    fractions=boundary_fraction(np.linspace(0,2*np.pi,16384,endpoint=False),outline,angular)
    ratio=float(np.mean(fractions**2));checks['irregular_contour_retains_at_least_90_percent']=ratio>=.9
    states={}
    for name,w,h,zoom,pan in [('fog-1600x1000',1600,1000,1,(0,0)),('fog-1920x1080',1920,1080,1,(0,0)),
                            ('fog-1280x720',1280,720,1,(0,0)),('fog-ultrawide',2560,1080,1,(0,0)),('fog-zoom-pan',1600,1000,1.6,(-30,30))]:
        base=np.array(Image.open(run/(name+'-base.png')).convert('RGBA')).astype(int)
        probe=np.array(Image.open(run/(name+'-probe.png')).convert('RGBA')).astype(int)
        distance,boundary=contour_field(w,h,outline,zoom,pan,angular)
        delta=np.max(np.abs(base[:,:,:3]-probe[:,:,:3]),axis=2)
        outside=distance>boundary+2
        max_outside=int(delta[outside].max())
        states[name]=dict(max_outside_probe_difference=max_outside,interior_probe_pixels=int(np.sum((delta>5)&~outside)))
        checks[name+'_opaque_outside_contour']=max_outside<=1 and bool(np.all(base[...,3]==255))
        checks[name+'_probe_reaches_interior']=states[name]['interior_probe_pixels']>1000
    out=dict(passed=all(checks.values()),checks=checks,mismatches=mismatches,states=states,
             retained_circle_area_ratio=ratio,revealed_area_m2=float(np.pi*1000**2*ratio),
             scope='Current real standalone widget captures and source/DLL fingerprints; no physical input or full gameplay acceptance claim')
    (run/'expanded-audit.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(out,ensure_ascii=False),flush=True)
    raise SystemExit(0 if out['passed'] else 1)

if __name__=='__main__':main()
