"""Render per-action contacts and true one-cycle gait samples for visual QA."""
import bpy,sys,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from preview import setup
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if args:jobs=[j for j in jobs if j['slug'] in args]
    for job in jobs:
        out=Path(job['output']);man=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
        bpy.ops.wm.open_mainfile(filepath=str(out/('AS_'+job['slug']+'.blend')))
        arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
        setup(arm,man['rig']['axis']['target_length_m'])
        scene=bpy.context.scene;scene.render.resolution_x=400;scene.render.resolution_y=300
        previews=out/'review';previews.mkdir(exist_ok=True)
        for c in man['clips']:
            arm.animation_data.action=bpy.data.actions[c['name']];scene.render.fps=c['fps']
            for frac in (0,.5,1):
                scene.frame_set(1+round((c['frames']-1)*frac))
                scene.render.filepath=str(previews/(c['suffix']+'_'+str(round(frac*100))+'.png'));bpy.ops.render.render(write_still=True)
            print('REVIEW '+job['slug']+' '+c['suffix'],flush=True)
        (previews/'render_record.json').write_text(json.dumps({'source':str(out/('AS_'+job['slug']+'.blend')),'actions':len(man['clips']),'views':['three-quarter'],'sample_fractions':[0,.5,1],'status':'rendered, awaiting visual assessment'},indent=2),encoding='utf-8')

if __name__=='__main__':main()
