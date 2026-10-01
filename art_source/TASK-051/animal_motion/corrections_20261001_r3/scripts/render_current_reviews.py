"""Update all pose thumbnails against the current paired mesh and source."""
import bpy,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,jobs,read,save,sha
from preview import setup

args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
for job in jobs(args):
    out=Path(job['output']);m=read(out/'animation_manifest.json');source=out/f'AS_{job["slug"]}.blend';digest=sha(source)
    bpy.ops.wm.open_mainfile(filepath=str(source));scene=bpy.context.scene;arm=next(o for o in scene.objects if o.type=='ARMATURE')
    setup(arm,m['rig']['axis'].get('body_reference_length_m',m['rig']['axis']['target_length_m']));scene.eevee.taa_render_samples=16
    images=[];folder=out/'review';folder.mkdir(exist_ok=True)
    for c in m['clips']:
        arm.animation_data.action=bpy.data.actions[c['name']];scene.render.fps=c['fps']
        for fraction in (0,.5,1):
            f=1+round((c['frames']-1)*fraction);scene.frame_set(f);path=folder/f'{c["suffix"]}_{round(fraction*100)}.png';scene.render.filepath=str(path)
            bpy.ops.render.render(write_still=True);images.append({'action':c['name'],'frame':f,'path':str(path)})
    save(folder/'source_record_20261001_r3.json',{'source_blend_sha256':digest,'images':images,'view':'current native paired mesh and baked source; three-quarter'})
    print('CURRENT_REVIEWS',job['slug'],len(images),flush=True)
