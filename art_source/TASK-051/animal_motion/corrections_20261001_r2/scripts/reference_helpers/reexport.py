"""Re-export edited sources with consistent FBX binding reference data."""
import bpy,json,sys,hashlib
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from author import export_fbx,reset
ROOT=Path(r'E:\AiAgent\XLingGame\Resource\Tripo\动物\动作\制作成果')

def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    jobs=json.loads((ROOT/'jobs.json').read_text(encoding='utf-8'))
    if args:jobs=[j for j in jobs if j['slug'] in args]
    for j in jobs:
        out=Path(j['output']);man=json.loads((out/'animation_manifest.json').read_text(encoding='utf-8'))
        bpy.ops.wm.open_mainfile(filepath=str(out/('AS_'+j['slug']+'.blend')))
        arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
        arm.animation_data.action=None;reset(arm);bpy.context.view_layer.update()
        export_fbx(arm,meshes,out/('SK_'+j['slug']+'.fbx'),True,False)
        man['fbx_units']='centimetres; identity object and root scales'
        meta={'game_id':j['game_id'],'run_id':j['run_id'],'task_kind':'motion','task_id':j['slug'],'skeletal_fbx_path':str(out/('SK_'+j['slug']+'.fbx'))}
        for c in man['clips']:
            scene=bpy.context.scene;scene.render.fps=c['fps'];scene.frame_start=1;scene.frame_end=c['frames'];arm.animation_data.action=bpy.data.actions[c['name']];scene.frame_set(1)
            export_fbx(arm,meshes,Path(c['file']),False,True)
            c['sha256']=hashlib.sha256(Path(c['file']).read_bytes()).hexdigest();c['fbx_bind_reference_proxy']=True
            meta[c['name']+'_path']=c['file']
        (out/'animation_manifest.json').write_text(json.dumps(man,ensure_ascii=False,indent=2),encoding='utf-8')
        (out/'meta.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
        print('REEXPORTED '+j['slug'],flush=True)

if __name__=='__main__':main()
