"""Actual editable source at the native engine's five compressed-pose times."""
import bpy,sys,numpy as np
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import REV,jobs,read,save,sha,backup_folder
args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
D=np.diag([1.,-1.,1.])
for j in jobs(args):
    out=backup_folder(j) if '--baseline' in args else Path(j['output']);m=read(out/'animation_manifest.json');source=out/f'AS_{j["slug"]}.blend';bpy.ops.wm.open_mainfile(filepath=str(source));arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');scene=bpy.context.scene;clips=[]
    for c in m['clips']:
        arm.animation_data.action=bpy.data.actions[c['name']];samples=[]
        for fraction in (0.,.25,.5,.75,1.):
            f=1+(c['frames']-1)*fraction;scene.frame_set(int(f),subframe=f-int(f));bones={}
            for p in arm.pose.bones:
                matrix=arm.matrix_world@p.matrix;rot=np.array(matrix.to_quaternion().to_matrix());pos=np.array(matrix.translation)*100
                bones[p.name]={'translation_cm':(D@pos).tolist(),'rotation_matrix':(D@rot@D).tolist()}
            samples.append({'fraction':fraction,'bones':bones})
        clips.append({'name':c['name'],'source_fbx_sha256':c['sha256'],'samples':samples})
    save(REV/('baseline' if '--baseline' in args else 'final')/f'{j["slug"]}_source_poses.json',{'slug':j['slug'],'source_blend_sha256':sha(source),'skeletal_fbx_sha256':sha(out/f'SK_{j["slug"]}.fbx'),'clips':clips,'coordinate_conversion':'metres to centimetres; Unreal handedness Y reflection; constant FBX per-bone basis correction verified separately'})
    print('SOURCE_POSES',j['slug'],len(clips),flush=True)
