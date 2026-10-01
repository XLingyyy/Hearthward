import bpy,sys,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from common import ROOT,REV,jobs,read,save,backup_folder
from audit_refinement import angle

for job in jobs(['wolf','hare','red_fox','goat']):
    old={};checks=[]
    for phase,out in [('old',backup_folder(job)),('new',REV/'candidate'/job['slug'])]:
        m=read(out/'animation_manifest.json');bpy.ops.wm.open_mainfile(filepath=str(out/f'AS_{job["slug"]}.blend'))
        arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');scene=bpy.context.scene
        for c in m['clips']:
            if c['kind']!='run':continue
            arm.animation_data.action=bpy.data.actions[c['name']]
            for f in range(1,c['frames']+1):
                scene.frame_set(f)
                for k,ns in m['rig']['chains'].items():
                    mat=arm.pose.bones[ns[3]].matrix
                    if phase=='old':old[(c['name'],f,k)]=(mat.translation.copy(),mat.to_quaternion())
                    else:
                        p,q=old[(c['name'],f,k)]
                        checks.append({'frame':f,'leg':k,'position_cm':(mat.translation-p).length*100,'rotation_deg':angle(q,mat.to_quaternion())})
    print('FEET',job['slug'],max(c['position_cm'] for c in checks),max(c['rotation_deg'] for c in checks),flush=True)
    save(REV/'candidate'/f'{job["slug"]}_feet.json',checks)
